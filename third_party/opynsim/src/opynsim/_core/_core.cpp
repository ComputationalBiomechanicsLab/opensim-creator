#include "_core.h"

#include <opynsim/_core/arrow.h>
#include <opynsim/_core/config.h>
#include <opynsim/_core/examples.h>
#include <opynsim/_core/graphics.h>
#include <opynsim/_core/tps3d.h>
#include <opynsim/_core/ui.h>

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/scope_exit.h>
#include <liboscar/utilities/string_helpers.h>
#include <libopynsim/platform/opynsim_app.h>
#include <libopynsim/data_frame.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>
#include <libopynsim/opynsim.h>
#include <libopynsim/symbol.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <new>
#include <mutex>
#include <ranges>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace opyn;

namespace nb = nanobind;
namespace rgs = std::ranges;

namespace {
    std::unique_ptr<OPynSimApp> g_lazy_loaded_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    template<typename CppType>
    struct PythonTypeMapper {
        using cpp_type = CppType;
        using python_type = CppType;

        static python_type to_python(cpp_type&& v) { return python_type{std::move(v)}; }
    };

    template<>
    struct PythonTypeMapper<osc::Vector3d> {
        using cpp_type = osc::Vector3d;
        using python_type = nb::ndarray<nb::numpy, double, nb::shape<3>>;

        static python_type to_python(cpp_type&& v) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
        {
            auto* data = new double[3]{v[0], v[1], v[2]};  // NOLINT(cppcoreguidelines-owning-memory)
            const std::array<size_t, 1> shape = {3};
            return {
                data,
                1,
                shape.data(),
                nb::capsule(data, [](void* p) noexcept { delete[] static_cast<double*>(p); })  // NOLINT(cppcoreguidelines-owning-memory)
            };
        }
    };

    template<typename... Ts>
    auto to_python_mapper_typelist(osc::Typelist<Ts...>)
    {
        return osc::Typelist<typename PythonTypeMapper<Ts>::python_type...>{};
    }

    using SupportedPythonOutputValues = decltype(to_python_mapper_typelist(std::declval<SupportedOutputValueTypes>()));
    using PythonOutputValue = osc::VariantOfTypelistElements<SupportedPythonOutputValues>;

    PythonOutputValue to_python_output(OutputValue&& output_value)
    {
        return std::visit(osc::Overload{
            []<typename T>(T&& v) -> PythonOutputValue { return PythonTypeMapper<T>::to_python(std::forward<T>(v)); }
        }, std::move(output_value));  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    }

    // Satisfied if `T` is an Arrow object (a C struct with a type-erased destructor).
    template<typename T>
    concept ArrowCObject = requires(T& value)
    {
        std::is_trivially_constructible_v<T>;
        std::is_trivially_destructible_v<T>;
        { value.release(&value) };
        { value.private_data } -> std::same_as<void*&>;
    };

    // A template function that can be instantiated to create an Arrow C release callback
    // that `delete`s the given `PrivateDataType`.
    template<ArrowCObject T, typename PrivateDataType>
    void private_data_releaser(T* ptr)
    {
        if (ptr->release) {
            delete static_cast<PrivateDataType*>(ptr->private_data);  // NOLINT(cppcoreguidelines-owning-memory)
            ptr->release = nullptr;
        }
    }

    // A template function that can be instantiated to create an Arrow C callback that
    // calls a `MemberFunction` of `PrivateData` with given `Args`.
    template<ArrowCObject T, typename PrivateData, auto MemberFunction, typename... Args>
    auto type_erased_callback(T* self, Args... args)
    {
        auto* pdata = static_cast<PrivateData*>(self->private_data);
        return (pdata->*MemberFunction)(args...);
    }

    // Represents a nullable owned handle to an `ArrowCObject`.
    template<ArrowCObject T>
    class ArrowHandle final {
    public:
        ArrowHandle() = default;
        ArrowHandle(const ArrowHandle&) = delete;
        ArrowHandle(ArrowHandle&& tmp) noexcept :
            ptr_{std::exchange(tmp.ptr_, nullptr)}
        {}
        ~ArrowHandle() noexcept
        {
            if (ptr_) {
                if (ptr_->release) {
                    ptr_->release(ptr_);
                }
                delete ptr_;
            }
        }
        ArrowHandle& operator=(const ArrowHandle&) = delete;
        ArrowHandle& operator=(ArrowHandle&& rhs) noexcept { std::swap(ptr_, rhs.ptr_); return *this; }

        T* release() { return std::exchange(ptr_, nullptr); }
        T& operator*() const { return *ptr_; }

    private:
        T* ptr_ = new T{};
    };

    template<ArrowCObject T>
    struct ArrowLifetimeWrapper final : T {
        ArrowLifetimeWrapper() : T{} {}
        ArrowLifetimeWrapper(const ArrowLifetimeWrapper&) = delete;
        ArrowLifetimeWrapper(ArrowLifetimeWrapper&&) noexcept = delete;
        ~ArrowLifetimeWrapper() noexcept { if (this->release) { this->release(this); }}
        ArrowLifetimeWrapper& operator=(const ArrowLifetimeWrapper&) = delete;
        ArrowLifetimeWrapper& operator=(ArrowLifetimeWrapper&&) noexcept = delete;
    };

    template<ArrowCObject T>
    nb::capsule to_capsule(ArrowHandle<T> handle, char const* name)
    {
        return nb::capsule(handle.release(), name, [](void* p) noexcept { delete static_cast<T*>(p); });  // NOLINT(cppcoreguidelines-owning-memory)
    }

    // Represents a contiguous sequence of `ArrowHandle<T>`s.
    template<ArrowCObject T>
    class ArrowChildren final {
    public:
        void reserve(size_t new_cap) { handles_.reserve(new_cap); }
        ArrowHandle<T>& emplace_back() { return handles_.emplace_back(); }
        int64_t n_children() const { return static_cast<int64_t>(handles_.size()); }
        T** children()
        {
            static_assert(sizeof(ArrowHandle<T>) == sizeof(T*));
            static_assert(alignof(ArrowHandle<T>) == alignof(T*));
            static_assert(std::is_standard_layout_v<ArrowHandle<T>>);
            return handles_.empty() ? nullptr : reinterpret_cast<T**>(std::launder(handles_.data()));
        }
    private:
        std::vector<ArrowHandle<T>> handles_;
    };

    template<ArrowCObject T, typename U>
    void arrow_assign(T&, const U&);

    template<ArrowCObject T>
    ArrowChildren<T> generate_children(const DataFrame& data_frame)
    {
        ArrowChildren<T> rv;
        rv.reserve(data_frame.size());
        for (const auto& series : data_frame) {
            auto& handle = rv.emplace_back();
            arrow_assign(*handle, series);
        }
        return rv;
    }

    std::optional<std::vector<char>> encode_attrs_to_binary_string(const DataFrame& data_frame)
    {
        const auto attrs = data_frame.attrs();
        if (attrs.empty()) {
            return std::nullopt;
        }

        // Figure out the string length (single allocation).
        const size_t buffer_len = [&attrs]
        {
            size_t rv = sizeof(int32_t);  // Field: the number of key-value pairs (int32).
            for (const auto& [k, v] : attrs) {
                rv += sizeof(int32_t);    // Field: number of bytes in key N (int32).
                rv += k.size();           // Field: key character bytes.
                rv += sizeof(int32_t);    // Field: the number of bytes in value N (int32).
                rv += v.size();           // Field: value character bytes.
            }
            return rv;
        }();

        // Converts the size of range `r` into a sequence of native-endian int32 bytes.
        const auto size_as_int32_bytes = [](const auto& r)
        {
            return std::bit_cast<std::array<char, sizeof(int32_t)>>(static_cast<int32_t>(rgs::size(r)));
        };

        // Create + encode the string
        std::vector<char> rv;
        rv.reserve(buffer_len);
        osc::append_range(rv, size_as_int32_bytes(attrs));
        for (const auto& [k, v] : attrs) {
            osc::append_range(rv, size_as_int32_bytes(k));
            osc::append_range(rv, k);
            osc::append_range(rv, size_as_int32_bytes(v));
            osc::append_range(rv, v);
        }
        return rv;
    }

    template<>
    void arrow_assign(ArrowSchema& lhs, const Series& series)
    {
        using PrivateData = Series;
        auto pdata = std::make_unique<PrivateData>(series);

        lhs.format = "g";  static_assert(std::same_as<Series::value_type, double>);
        lhs.name = pdata->name().c_str();
        lhs.metadata = nullptr;
        lhs.flags = int64_t{0};
        lhs.n_children = int64_t{0};
        lhs.children = nullptr;
        lhs.dictionary = nullptr;
        lhs.release = private_data_releaser<ArrowSchema, PrivateData>;
        lhs.private_data = pdata.release();
    }

    void arrow_assign(ArrowSchema& lhs, const DataFrame& data_frame)
    {
        struct PrivateData {
            explicit PrivateData(DataFrame data_frame) : data_frame{std::move(data_frame)} {}

            DataFrame data_frame;
            ArrowChildren<ArrowSchema> children = generate_children<ArrowSchema>(data_frame);
            std::optional<std::vector<char>> metadata = encode_attrs_to_binary_string(data_frame);
        };
        auto pdata = std::make_unique<PrivateData>(data_frame);

        lhs.format = "+s";
        lhs.name = "";
        lhs.metadata = pdata->metadata ? pdata->metadata->data() : nullptr;
        lhs.flags = int64_t{0};
        lhs.n_children = pdata->children.n_children();
        lhs.children = pdata->children.children();
        lhs.dictionary = nullptr;
        lhs.release = private_data_releaser<ArrowSchema, PrivateData>;
        lhs.private_data = pdata.release();
    }

    template<>
    void arrow_assign(ArrowArray& lhs, const Series& series)
    {
        struct PrivateData {
            explicit PrivateData(Series series) : series{std::move(series)} {}

            Series series;
            std::vector<const void*> buffers = {nullptr, series.data()};  // [validity bitmap (unused), data]
        };
        auto pdata = std::make_unique<PrivateData>(series);

        lhs.length = static_cast<int64_t>(pdata->series.size());
        lhs.null_count = 0;
        lhs.offset = 0;
        lhs.n_buffers = static_cast<int64_t>(pdata->buffers.size());
        lhs.buffers = pdata->buffers.data();
        lhs.n_children = 0;
        lhs.children = nullptr;
        lhs.dictionary = nullptr;
        lhs.release = private_data_releaser<ArrowArray, PrivateData>;
        lhs.private_data = pdata.release();
    }

    template<>
    void arrow_assign(ArrowArray& lhs, const DataFrame& data_frame)
    {
        struct PrivateData {
            explicit PrivateData(DataFrame data_frame) : data_frame{std::move(data_frame)} {}

            DataFrame data_frame;
            ArrowChildren<ArrowArray> children = generate_children<ArrowArray>(data_frame);
            std::vector<const void*> buffers = {nullptr};  // [validity bitmap (unused)]
        };
        auto pdata = std::make_unique<PrivateData>(data_frame);

        lhs.length = data_frame.height();
        lhs.null_count = 0;
        lhs.offset = 0;
        lhs.n_buffers = pdata->buffers.size();
        lhs.buffers = pdata->buffers.data();
        lhs.n_children = pdata->children.n_children();
        lhs.children = pdata->children.children();
        lhs.dictionary = nullptr;
        lhs.release = private_data_releaser<ArrowArray, PrivateData>;
        lhs.private_data = pdata.release();
    }

    template<>
    void arrow_assign(ArrowArrayStream& lhs, const DataFrame& data_frame)
    {
        struct PrivateData {
            explicit PrivateData(DataFrame data_frame) : data_frame_{std::move(data_frame)} {}

            int get_schema(ArrowSchema* out)
            {
                last_error_.clear();

                try {
                    arrow_assign(*out, data_frame_);
                    return 0;
                }
                catch (const std::bad_alloc& ex) {
                    last_error_ = ex.what();
                    return ENOMEM;
                }
            }

            int get_next(ArrowArray* out)
            {
                last_error_.clear();

                if (std::exchange(array_already_emitted_, true)) {
                    out->release = nullptr;  // Spec: end of stream.
                    return 0;
                }

                try {
                    arrow_assign(*out, data_frame_);
                    return 0;
                }
                catch (const std::bad_alloc& ex) {
                    last_error_ = ex.what();
                    return ENOMEM;
                }
            }

            const char* get_last_error() const { return last_error_.data(); }

            DataFrame data_frame_;
            bool array_already_emitted_ = false;
            std::string last_error_;
        };
        auto pdata = std::make_unique<PrivateData>(data_frame);

        lhs.get_schema     =  type_erased_callback<ArrowArrayStream, PrivateData, &PrivateData::get_schema, ArrowSchema*>;
        lhs.get_next       =  type_erased_callback<ArrowArrayStream, PrivateData, &PrivateData::get_next, ArrowArray*>;
        lhs.get_last_error =  type_erased_callback<ArrowArrayStream, PrivateData, &PrivateData::get_last_error>;
        lhs.release        = private_data_releaser<ArrowArrayStream, PrivateData>;
        lhs.private_data   = pdata.release();
    }

    DataFrame construct_dataframe(ArrowArrayStream& stream)
    {
        ArrowLifetimeWrapper<ArrowSchema> schema;
        if (stream.get_schema(&stream, &schema) != 0) {
            throw std::runtime_error{"Failed to get `ArrowSchema` from the provided `ArrowArrayStream`"};
        }

        // Validate top-level schema.
        if (std::string_view{schema.format} != "+s") {
            throw std::runtime_error{"OPynSim can only parse a table (a struct, '+s') containing float64 columns into a `DataFrame` right now - you may need to flatten and convert your columns accordingly (sorry - work in progress!)"};
        }

        // Get column names and validate each column's data type.
        std::vector<std::string> column_names;
        column_names.reserve(schema.n_children);
        for (int64_t i = 0; i < schema.n_children; ++i) {
            auto& child = schema.children[i];
            std::string_view format{child->format};
            if (format != "g") {
                std::stringstream ss;
                ss << "Child " << i << " in the provided data table has an invalid type '" << format << "' (expected: 'g' - float64): OPynSim can only parse a flat table containing float64 series right now - you may need to convert your data accordingly (sorry - work in progress!)";
                throw std::runtime_error{std::move(ss).str()};
            }
            column_names.push_back(child->name ? std::string{child->name} : std::string{});
        }

        // Read `ArrowArray` (chunks) into local vector.
        std::vector<std::vector<double>> column_data;
        column_data.resize(schema.n_children);
        while (true) {
            ArrowLifetimeWrapper<ArrowArray> array;
            OSC_ASSERT(array.release == nullptr);
            if (stream.get_next(&stream, &array) != 0) {
                std::stringstream ss;
                ss << "Error encountered when reading an array stream: " << stream.get_last_error(&stream);
                throw std::runtime_error{std::move(ss).str()};
            }
            if (array.release == nullptr) {
                break;  // This is how the API communicates "done"
            }

            OSC_ASSERT(array.n_children == column_data.size() && "The number of children in an array stream doesn't match the provided schema");

            // Read each child of doubles (validated above)
            for (size_t i = 0; i < column_data.size(); ++i) {
                auto& chunk = array.children[i];
                if (chunk->length > 0) {
                    std::span source_span{
                        static_cast<const double*>(chunk->buffers[1]) + chunk->offset,
                        static_cast<size_t>(chunk->length)
                    };
                    column_data[i].insert(column_data[i].end(), source_span.begin(), source_span.end());
                }
            }
        }

        // Read metadata
        std::unordered_map<std::string, std::string> metadata;
        if (schema.metadata) {
            const auto read_int32_as_size_t = [](const char*& cursor)
            {
                std::array<char, sizeof(int32_t)> copied_bytes{};
                rgs::copy(std::span<const char, sizeof(int32_t)>{cursor, sizeof(int32_t)}, copied_bytes.begin());
                cursor += sizeof(int32_t);
                return static_cast<size_t>(std::bit_cast<int32_t>(copied_bytes));
            };
            const auto read_string = [&read_int32_as_size_t](const char*& cursor)
            {
                const size_t len = read_int32_as_size_t(cursor);
                const std::string rv{cursor, cursor + len};
                cursor += len;
                return rv;
            };

            const char* cursor = schema.metadata;
            const size_t num_kv_pairs = read_int32_as_size_t(cursor);
            metadata.reserve(num_kv_pairs);
            for (size_t i = 0; i < num_kv_pairs; ++i) {
                auto k = read_string(cursor);
                auto v = read_string(cursor);
                metadata.try_emplace(std::move(k), std::move(v));
            }
        }

        return DataFrame{std::move(column_names), std::move(column_data), std::move(metadata)};
    }

    void register_dataframe_class(nb::module_& m)
    {
        nb::class_<DataFrame> cls(m, "DataFrame", R"(
            Represents data as a table containing rows and columns, with metadata (:meth:`attrs`).

            :class:`DataFrame` only supports storing ``float64`` data. It enforces no constraints
            on the number/order/labels of columns, or the values it contains. However, other
            APIs in OPynSim may enforce stronger constraints (e.g. "The :class:`DataFrame` supplied to
            this solver must contain a series named ``time`` with strongly monotonically increasing
            values"). In general, those constraints are checked at runtime and produce a validation
            error. Callers are expected to handle filtering/cleaning/fixing their :class:`DataFrame`\s
            accordingly.

            :class:`DataFrame`'s functionality is limited to being *just* good enough for common
            OPynSim-related tasks. However, it the `Arrow PyCapsule Protocol <https://arrow.apache.org/docs/format/CDataInterface/PyCapsuleInterface.html>`_,
            which lets you import/export :class:`DataFrame`\s to more-comprehensive libraries (e.g.
            :meth:`DataFrame.from_arrow`, ``pandas.DataFrame.from_arrow``, ``polars.from_arrow``,
            ``pyarrow.table.__init__``). If you want to do some fancy data manipulation, we
            recommend using one of those libraries.
        )");
        cls.def_static(
            "from_arrow",
            [](nb::object data)
            {
                auto arrow_c_stream_method = nb::getattr(std::move(data), "__arrow_c_stream__");
                auto method_rv = arrow_c_stream_method();
                auto stream_capsule = nb::cast<nb::capsule>(method_rv);

                const char* capsule_name = stream_capsule.name();
                if (not (capsule_name and std::string_view{capsule_name} == "arrow_array_stream")) {
                    throw nb::value_error("Expected capsule name 'arrow_array_stream', but got something else");
                }

                auto* stream = static_cast<ArrowArrayStream*>(stream_capsule.data());
                if (stream == nullptr) {
                    throw nb::value_error("Capsule from '__arrow_c_stream__' contains a null pointer");
                }

                return construct_dataframe(*stream);
            },
            nb::arg("data"),
            R"(
                Constructs a `DataFrame` from an array-like Arrow object.

                This function accepts any Arrow-compatible array-like object implementing the array-streaming
                part of the `Arrow PyCapsule Protocol <https://arrow.apache.org/docs/format/CDataInterface/PyCapsuleInterface.html>`_
                (i.e. having an ``__arrow_c_stream__`` method).

                Notably, the ``DataFrame`` classes of popular third-party dataframe libraries such
                as `Pandas <https://pandas.pydata.org/>`_ (>= v2.2.0) and `Polars <https://pola.rs/>`_ (>= v0.20.4)
                implement the protocol, meaning you can use this function to optimally import those ``DataFrame``\s into
                an OPynSim :class:`DataFrame`.
            )"
        );
        cls.def(
            nb::init{},
            R"(
                Constructs an empty ``DataFrame``.

                At the moment (WIP), the only way to construct a ``DataFrame`` that contains data is
                via methods like :meth:`opynsim.read_sto`.
            )"
        );
        cls.def("__repr__", osc::stream_to_string<DataFrame>);
        cls.def("__str__", osc::stream_to_string<DataFrame>);
        cls.def_prop_ro(
            "attrs",
            &DataFrame::attrs,
            R"(
                Returns the attributes (metadata) associated with ``self``.

                In some cases, attributes can affect the behavior of functions that read data
                from :class:`DataFrame`\s. Notably, functions like :meth:`opynsim.Model.states_from_data_frame` look
                for attributes like ``'inDegrees'`` to perform on-the-fly degrees-to-radians conversions
                on legacy data files.
            )"
        );
        cls.def(
            "__arrow_c_stream__",
            [](const DataFrame& data_frame,
               [[maybe_unused]] const std::optional<nb::capsule>& requested_schema = std::nullopt)
            {
                ArrowHandle<ArrowArrayStream> stream;
                arrow_assign(*stream, data_frame);
                return to_capsule(std::move(stream), "arrow_array_stream");
            },
            nb::arg("requested_schema") = std::nullopt,
            R"(
                Exports this ``DataFrame`` as an ``ArrowSchema`` (see: `Apache Arrow PyCapsule Interface <https://arrow.apache.org/docs/dev/format/CDataInterface/PyCapsuleInterface.html>`_).

                This is a low-level interface that other dataframe libraries (e.g. `Pandas <https://pandas.pydata.org/>`_
                and `Polars <https://pola.rs/>`_) can use to natively read :class:`DataFrame`\s. For
                example, ``polars.DataFrame.__init__`` accepts :class:`DataFrame`\s because it implements this
                method, as does ``pandas.DataFrame.from_arrow``.

                **Note**: This method also exports metadata (:meth:`attrs`), but third-party libraries handle
                metadata inconsistently. At time of writing, `PyArrow <https://arrow.apache.org/docs/python/index.html>`_
                encodes metadata into its table schema, but `Pandas <https://pandas.pydata.org/>`_ and `Polars <https://pola.rs/>`_
                drop it. Therefore, we recommend that callers propagate metadata manually, or adjust their
                :class:`DataFrame` to no need the metadata (e.g. use :meth:`Model.convert_data_frame_to_radians`
                so that ``inDegrees`` isn't needed).

                Args:
                    requested_schema: An optional Arrow schema capsule (currently ignored).

                Returns:
                    A ``PyCapsule`` called "arrow_array_stream" containing a C ``ArrowArrayStream`` struct.
            )"
        );
        cls.def_prop_ro(
            "shape",
            &DataFrame::shape,
            R"(
                Returns the shape of the ``DataFrame``.
            )"
        );
        cls.def(
            "to_pandas",
            [](const DataFrame& data_frame)
            {
                // Lazily import `pandas` (OPynSim isn't dependent on it) and perform
                // runtime lookups to figure out how to export the data into a
                // `pandas.DataFrame`.

                nb::module_ pd = nb::module_::import_("pandas");
                nb::object DataFrame = pd.attr("DataFrame");
                nb::object from_arrow = DataFrame.attr("from_arrow");
                nb::object data_frame_obj = nb::cast(&data_frame, nb::rv_policy::reference);
                return from_arrow(data_frame_obj);
            },
            R"(
                Returns a ``pandas.DataFrame`` constructed from ``self``.

                The ``pandas`` module is lazily ``import``\ed when this method is called. It's
                expected that the caller's environment supplies a version of ``pandas`` that is compatible with
                the Arrow API (>= v2.2.0). This may require additionally supplying ``pyarrow``, which
                ``pandas`` may internally use to implement the API.

                See also: :meth:`__arrow_c_stream__` and :meth:`from_arrow`.
            )"
        );
        cls.def(
            "to_polars",
            [](const DataFrame& data_frame)
            {
                // Lazily import `polars` (OPynSim isn't dependent on it) and perform
                // runtime lookups to figure out how to export the data into a
                // `polars.DataFrame`.

                nb::module_ pd = nb::module_::import_("polars");
                nb::object from_arrow = pd.attr("from_arrow");
                nb::object data_frame_obj = nb::cast(&data_frame, nb::rv_policy::reference);
                return from_arrow(data_frame_obj);
            },
            R"(
                Returns a ``polars.DataFrame`` constructed from ``self``.

                The ``polars`` module is lazily ``import``\ed when this method is called. It's
                expected that the callers have installed a version of ``polars`` that is compatible with
                the Arrow API (>= v0.20.4).

                See also: :meth:`__arrow_c_stream__` and :meth:`from_arrow`.
            )"
        );
    }

    void register_symbol_class(nb::module_& m)
    {
        nb::class_<Symbol> symbol_class(
            m,
            "Symbol",
            R"(
                Represents an immutable, cheap-to-use, readable symbol.

                Symbols are extensively used by the OPynSim API to accelerate associative lookups. They are the
                middle-ground between fast, but hard to read/introspect, integer handles and slow, simpler string
                handles.

                From Python code's point of view, symbols should be seen as string-like handles that OPynSim
                accepts/emits. You can safely store symbols independently of any larger data structure, and
                interchange them across your entire Python codebase, without having to worry about object
                lifetimes. OPynSim's native code uses runtime-checked associative lookups, rather than pointers, to
                ensure that the Python API is memory-safe and can provide suitable feedback whenever a lookup fails.
            )"
        );
        symbol_class.def(
            nb::init<std::string_view>(),
            nb::arg("id"),
            R"(
                Constructs a symbol from a Python string.
            )"
        );
        symbol_class.def(
            "__str__",
            [](const Symbol& symbol) { return static_cast<std::string>(symbol); },
            "Converts this symbol into a Python :class:`str`"
        );
        symbol_class.def("__repr__", [](const Symbol& self) { return std::string("Symbol(\"") + std::string(self) + "\")"; });
        symbol_class.def("__hash__", [](const Symbol& self) { return std::hash<Symbol>{}(self); });
        symbol_class.def("__eq__",   [](const Symbol& self, const Symbol& other)  { return self == other; });
        symbol_class.def("__eq__",   [](const Symbol& self, std::string_view rhs) { return self == rhs; });
        symbol_class.def("__contains__", [](const Symbol& self, std::string_view rhs) { return static_cast<std::string_view>(self).find(rhs) != std::string_view::npos; });
        nb::implicitly_convertible<std::string_view, Symbol>();
    }

    void register_model_specification_class(nb::module_& m)
    {
        nb::class_<ModelSpecification> model_specification_class(
            m,
            "ModelSpecification",
            R"(
                A high-level specification for a :class:`Model`.

                A :class:`ModelSpecification` is what Python code can manipulate, scale, and customize
                before passing it to :meth:`compile`, which returns a readonly :class:`Model`.

                Notes:
                    OPynSim's API design separates the specification of a model (:class:`ModelSpecification`)
                    from its validated, assembled, and optimized simulation representation (:class:`Model`) to ensure
                    that the compilation process (:meth:`compile`) can freeze and optimize internal
                    datastructures at a single point in the process.
            )"
        );
        model_specification_class.def(nb::init<>());  // Define default constructor
        model_specification_class.def(
            "compile",
            &ModelSpecification::compile,
            R"(
                Compiles this :class:`ModelSpecification` into a :class:`Model`.

                The compilation process:

                - Validates this :class:`ModelSpecification`'s components (properties, subcomponents,
                  and sockets), throwing an exception if the specification is invalid in some way.
                - Assembles a physics system from the validated specification, throwing an exception
                  if the physics system cannot be assembled (e.g. if the contains impossible-to-satisfy
                  joints, or invalid muscle definitions).

                Raises:
                    Exception: If the compilation process failed in some way. It is assumed that
                        the provided :class:`ModelSpecification` is valid.
            )"
        );
    }

    void register_model_class(nb::module_& m)
    {
        nb::class_<Model> model_class(
            m,
            "Model",
            R"(
                A compiled, ready-to-simulate, model of a physics system.

                A :class:`Model` can only be created from a :class:`ModelSpecification` via the
                :meth:`ModelSpecification.compile` function. Therefore, editing a :class:`Model` requires
                editing its associated :class:`ModelSpecification` and recompiling it to create a
                new :class:`Model`.
            )"
        );
        model_class.def_prop_ro(
            "num_coordinates",
            &Model::num_coordinates,
            R"(
                Returns the number of coordinates in the model.

                A coordinate represents a single degree of freedom (DoF) in the model, such as a joint angle,
                translation, or rotational parameter that contributes to the configuration/pose of a model.
            )"
        );
        model_class.def_prop_ro(
            "coordinates",
            &Model::coordinates,
            R"(
                Returns a list of all the coordinates in the model.
            )"
        );
        model_class.def(
            "initial_state",
            &Model::initial_state,
            nb::kw_only{},
            nb::arg("realized_to") = ModelStateStage::instance,
            R"(
                Returns a :class:`ModelState` that represents the initial (default) state of this :class:`Model`.

                The initial state of a :class:`Model` is dictated by the :class:`ModelSpecification` used to
                compile it. For example, if a translational coordinate in the specification had a ``default_value``
                of ``1.0`` then that would be written into the :class:`ModelState` returned by this function.

                The returned :class:`ModelState` will be realized to at least ``realized_to`` as-if by calling
                :meth:`Model.realize` on it.
            )"
        );
        model_class.def(
            "column_to_state_variable_mappings",
            &Model::column_to_state_variable_mappings,
            nb::arg("data_frame"),
            R"(
                Returns associative mappings between the names of columns in
                ``data_frame`` and state variables in this ``Model``, where
                the correspondence can be found.

                This mapping uses a variety of heuristics, including (e.g.)
                accounting for legacy column headers supported by earlier
                files in SIMM and OpenSim. It is how :meth:`states_from_data_frame`
                maps dataframes into :class:`ModelState`\s, so this method can be
                useful for debugging why states aren't being read correctly.
            )"
        );
        model_class.def(
            "rotational_columns_in",
            &Model::rotational_columns_in,
            nb::arg("data_frame"),
            R"(
                Returns the names of columns in ``data_frame`` that can be
                mapped to rotational state variables in this ``Model`` in
                the column-order of ``data_frame``.

                This is how :meth:`states_from_data_frame` automatically converts
                degrees to radians when ``data_frame.attrs["inDegrees"] == "yes"``,
                so it can be useful for debugging why states aren't
                being read correctly.
            )"
        );
        model_class.def(
            "convert_data_frame_to_radians",
            &Model::convert_data_frame_to_radians,
            nb::arg("data_frame"),
            R"(
                Returns a new ``DataFrame`` with all rotational columns converted to radians (if applicable).

                If ``data_frame.attrs["inDegrees"] != "yes"``, returns an identical clone of ``data_frame``.
                Otherwise, creates a new :class:`DataFrame` where :meth:`rotational_columns_in` is used
                to scale all rotational columns from degrees to radians (other columns are left
                unmodified). The ``"inDegrees"`` key is cleared from the returned :class:`DataFrame`\'s
                attributes.
            )"
        );
        model_class.def(
            "states_from_data_frame",
            &Model::states_from_data_frame,
            nb::arg("data_frame"),
            nb::kw_only{},
            nb::arg("realized_to") = ModelStateStage::instance,
            R"(
                Returns :class:`ModelStates` constructed from ``data_frame``.

                Columns in ``data_frame`` will be mapped to state variables in
                this ``Model`` (see: :meth:`column_to_state_variable_mappings`). Each
                row in ``data_frame`` constructs one :class:`ModelState` in the returned
                :class:`ModelStates`, in row-order.

                If ``data_frame.attrs["inDegrees"] == "yes"`` then rotational columns
                in ``data_frame`` will be automatically converted into radians internally
                (see: :meth:`rotational_columns_in`). This is to support :class:`DataFrame`\s
                loaded from legacy data sources.

                Each of the returned :class:`ModelState`\s will be realized to at
                least ``realized_to`` as-if by calling :meth:`Model.realize` on
                each of them.
            )"
        );
        model_class.def(
            "realize",
            &Model::realize,
            nb::arg("model_state"),
            nb::arg("model_state_stage"),
            R"(
                Realizes ``model_state`` to the desired ``model_state_stage``, which modifies
                ``model_state`` in-place.

                "Realization" of the state involves taking a new set of values from the :class:`ModelState`
                and performing computations that those new values enable. Realization is performed
                in-order one :class:`ModelStateStage` at time. For example, :attr:`ModelStateStage.POSITION`
                is realized before :attr:`ModelStateStage.VELOCITY`,then :attr:`ModelStateStage.DYNAMICS`,
                and so on.
            )"
        );
        model_class.def(
            "get_coordinate_value",
            &Model::get_coordinate_value,
            nb::arg("model_state"),
            nb::arg("coordinate"),
            R"(
                Returns the value of the corresponding state variable in ``model_state`` for the
                coordinate identified by ``coordinate``.
            )"
        );
        model_class.def(
            "set_coordinate_value",
            &Model::set_coordinate_value,
            nb::arg("model_state"),
            nb::arg("coordinate"),
            nb::arg("value"),
            R"(
                Sets corresponding state variable in ``model_state`` for the coordinate identified by
                ``coordinate`` to ``value``.

                Changing the value of a coordinate changes ``model_state``'s :class:`ModelStateStage` to
                :attr:`ModelStateStage.POSITION`. Therefore, you may need to use :meth:`realize` to
                re-realize the state to a later stage if you intend on using the state with a method that
                requires a later stage (e.g. rendering).
            )"
        );
        model_class.def_prop_ro(
            "num_outputs",
            &Model::num_outputs,
            R"(
                Returns the number of outputs the model has.
            )"
        );
        model_class.def_prop_ro(
            "outputs",
            &Model::outputs,
            R"(
                Returns a list of all outputs the model has.
            )"
        );
        model_class.def(
            "get_output_value",
            [](const Model& model, const ModelState& model_state, const Symbol& output)
            {
                return to_python_output(model.get_output_value(model_state, output));
            },
            nb::arg("model_state"),
            nb::arg("output"),
            R"(
                Returns the value of ``output`` for the given ``model_state``.
            )"
        );
    }

    void register_model_state_class(nb::module_& m)
    {
        nb::class_<ModelState> model_state_class(
            m,
            "ModelState",
            R"(
                Represents a single state of a :class:`Model`.

                A :class:`ModelState` bundles together the state variables, cache variables, and other information
                necessary to describe a single state of a :class:`Model`. :class:`Model`\s can read/manipulate
                :class:`ModelState`\s in order to :meth:`Model.realize` the state to a later stage
                (e.g. as part of forward integration) or read outputs values. However, :class:`ModelState`\s may
                also be created, read, and manipulated by downstream Python code and other utilities in OPynSim.
            )"
        );
        model_state_class.def_prop_ro(
            "stage",
            &ModelState::stage,
            R"(
                Returns the current :class:`ModelStateStage` of the state.

                Notes:
                    A state may be realized to a later stage with :meth:`Model.realize`.
            )"
        );
    }

    void register_model_states_class(nb::module_& m)
    {
        nb::class_<ModelStates> cls(
            m,
            "ModelStates",
            R"(
                Represents a sequence of :class:`ModelState`\s.

                ``ModelStates`` is typically returned from functions that produce sequences
                of :class:`ModelState`\s, such as :meth:`Model.states_from_data_frame`. The
                API of ``ModelStates`` is list-like, meaning downstream code can iterate over
                each ``ModelState``, use ``len`` with it, randomly access a state with ``model_states[idx]``
                and so on.
            )"
        );
        cls.def(nb::init<>());
        cls.def("__len__", &ModelStates::size);
        cls.def("__getitem__", [](ModelStates& states, ptrdiff_t idx)
        {
            if (idx < 0) {
                idx += static_cast<ptrdiff_t>(states.size());
            }
            return states.handle_at(idx);
        });
        cls.def("__getitem__", [](ModelStates& states, const nb::slice& slice)
        {
            const auto [start, stop, step, slice_length] = slice.compute(states.size());
            ModelStates rv;
            rv.reserve(slice_length);
            for (size_t i = 0; i < slice_length; ++i) {
                const auto cur = static_cast<size_t>(start + (static_cast<decltype(step)>(i)*step));
                rv.handle_push_back(states.handle_at(cur));
            }
            return rv;
        });
        cls.def("to_list", &ModelStates::to_handle_list);
    }

    void register_model_state_stage_class(nb::module_& m)
    {
        static_assert(osc::num_options<ModelStateStage>() == 9);
        nb::enum_<ModelStateStage> model_state_stage_class(
            m,
            "ModelStateStage",
            R"(
                Represents a stage of state realization (computation).

                When calling methods like :meth:`Model.realize`, a :class:`ModelState` is
                realized in-order through each :class:`ModelStateStage`, starting at the lowest
                stage and ending at the highest stage.

                Each time a :class:`ModelState` advances through a :class:`ModelStateStage`, more
                information is available in the state. For example, after a :class:`ModelState` is
                realized to :attr:`ModelStateStage.POSITION`, positional quantities such as the positions
                of bodies and offset frames are known, and any positional output on the associated
                :class:`Model` can then extract that information from the :class:`ModelState`.

                For convenience, the :mod:`opynsim` module defines aliases for each :class:`ModelStateStage`:

                - :attr:`opynsim.STAGE_TOPOLOGY` -> :attr:`opynsim.ModelStateStage.TOPOLOGY`
                - :attr:`opynsim.STAGE_MODEL` -> :attr:`opynsim.ModelStateStage.MODEL`
                - :attr:`opynsim.STAGE_INSTANCE` -> :attr:`opynsim.ModelStateStage.INSTANCE`
                - :attr:`opynsim.STAGE_TIME` -> :attr:`opynsim.ModelStateStage.TIME`
                - :attr:`opynsim.STAGE_POSITION` -> :attr:`opynsim.ModelStateStage.POSITION`
                - :attr:`opynsim.STAGE_VELOCITY` -> :attr:`opynsim.ModelStateStage.VELOCITY`
                - :attr:`opynsim.STAGE_DYNAMICS` -> :attr:`opynsim.ModelStateStage.DYNAMICS`
                - :attr:`opynsim.STAGE_ACCELERATION` -> :attr:`opynsim.ModelStateStage.ACCELERATION`
                - :attr:`opynsim.STAGE_REPORT` -> :attr:`opynsim.ModelStateStage.REPORT`
            )"
        );
        model_state_stage_class.value("TOPOLOGY",     ModelStateStage::topology,     "System topology known.");
        model_state_stage_class.value("MODEL",        ModelStateStage::model,        "Modelling choices have been made.");
        model_state_stage_class.value("INSTANCE",     ModelStateStage::instance,     "Physical parameters have been set.");
        model_state_stage_class.value("TIME",         ModelStateStage::time,         "Time has advanced and state variables have new values, but no derived information has been calculated.");
        model_state_stage_class.value("POSITION",     ModelStateStage::position,     "The spatial positions of all bodies are known.");
        model_state_stage_class.value("VELOCITY",     ModelStateStage::velocity,     "The spatial velocities of all bodies are known.");
        model_state_stage_class.value("DYNAMICS",     ModelStateStage::dynamics,     "The force acting on each body is known, along with total kinetic/potential energy.");
        model_state_stage_class.value("ACCELERATION", ModelStateStage::acceleration, "The time derivatives of all continuous state variables are known.");
        model_state_stage_class.value("REPORT",       ModelStateStage::report,       "Additional variables useful for output are known");

        // Define convenience aliases for the enum
        m.attr("STAGE_TOPOLOGY")     = model_state_stage_class.attr("TOPOLOGY");
        m.attr("STAGE_MODEL")        = model_state_stage_class.attr("MODEL");
        m.attr("STAGE_INSTANCE")     = model_state_stage_class.attr("INSTANCE");
        m.attr("STAGE_TIME")         = model_state_stage_class.attr("TIME");
        m.attr("STAGE_POSITION")     = model_state_stage_class.attr("POSITION");
        m.attr("STAGE_VELOCITY")     = model_state_stage_class.attr("VELOCITY");
        m.attr("STAGE_DYNAMICS")     = model_state_stage_class.attr("DYNAMICS");
        m.attr("STAGE_ACCELERATION") = model_state_stage_class.attr("ACCELERATION");
        m.attr("STAGE_REPORT")       = model_state_stage_class.attr("REPORT");
    }

    void register_readers(nb::module_& m)
    {
        m.def(
            "read_osim",
            [](const std::filesystem::path& source) { return opyn::read_osim(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`ModelSpecification` parsed from an `.osim` file on the
                caller's filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_sto",
            [](const std::filesystem::path& source) { return opyn::read_sto(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`DataFrame` parsed from an ``.sto`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_mot",
            [](const std::filesystem::path& source) { return opyn::read_mot(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`DataFrame` parsed from an ``.mot`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_trc",
            [](const std::filesystem::path& source) { return opyn::read_trc(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`DataFrame` parsed from an ``.trc`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_csv",
            [](const std::filesystem::path& source) { return opyn::read_csv(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`DataFrame` parsed from an ``.csv`` file on the caller's
                filesystem.

                The CSV file must have a header section, delimited by 'endheader`. This usually
                necessitates adding an `endheader` entry just above the header row (TODO: this
                limitation was inherited from OpenSim and shouldn't be a thing long-term).

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_vtp",
            [](const std::filesystem::path& source) { return opyn::read_vtp(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`graphics.Mesh` parsed from a ``.vtp`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_obj",
            [](const std::filesystem::path& source) { return opyn::read_obj(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`graphics.Mesh` parsed from a ``.obj`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_stl",
            [](const std::filesystem::path& source) { return opyn::read_obj(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`graphics.Mesh` parsed from a ``.stl`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_png",
            [](const std::filesystem::path& source) { return opyn::read_png(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`graphics.Texture2D` parsed from a ``.png`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_jpeg",
            [](const std::filesystem::path& source) { return opyn::read_jpeg(source); },
            nb::arg("source"),
            R"(
                Returns a :class:`graphics.Texture2D` parsed from a ``.jpeg`` file on the caller's
                filesystem.

                Raises:
                    RuntimeError: If the file cannot be found, read, or is invalid.
            )"
        );

        m.def(
            "read_jpg",
            [](const std::filesystem::path& source) { return opyn::read_jpg(source); },
            nb::arg("source"),
            "An alias for :func:`read_jpeg`"
        );
    }
}

opyn::OPynSimApp& opyn::get_lazy_loaded_opynsim_app()
{
    if (not g_lazy_loaded_app) {
        g_lazy_loaded_app = std::make_unique<OPynSimApp>();
    }
    return *g_lazy_loaded_app;
}

void opyn::destroy_lazy_loaded_opynsim_app()
{
    g_lazy_loaded_app.reset();
}

NB_MODULE(_core, _core_module)  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
{
    // Install an exit handler that cleans up any lazy-loaded application state
    // when the Python interpreter shuts down
    nb::module_::import_("atexit").attr("register")(nb::cpp_function(&destroy_lazy_loaded_opynsim_app));

    // Initialize `config` submodule (also initializes the `opynsim` C++ API, logging, etc.).
    {
        auto config_submodule = _core_module.def_submodule("config");
        init_config_submodule(config_submodule);
    }

    // Initialize `examples` submodule.
    {
        auto examples_submodule = _core_module.def_submodule("examples");
        init_examples_submodule(examples_submodule);
    }

    // Initialize `graphics` submodule.
    {
        auto graphics_submodule = _core_module.def_submodule("graphics");
        init_graphics_submodule(graphics_submodule);
    }

    // Initialize `tps3d` submodule.
    {
        auto tps3d_submodule = _core_module.def_submodule("tps3d");
        init_tps3d_submodule(tps3d_submodule);
    }

    // Initialize `ui` submodule.
    {
        auto ui_submodule = _core_module.def_submodule("ui");
        init_ui_submodule(ui_submodule);
    }

    // Initialize top-level functions/classes
    register_symbol_class(_core_module);
    register_model_specification_class(_core_module);
    register_model_state_stage_class(_core_module);
    register_model_class(_core_module);
    register_model_state_class(_core_module);
    register_model_states_class(_core_module);
    register_dataframe_class(_core_module);
    register_readers(_core_module);
}
