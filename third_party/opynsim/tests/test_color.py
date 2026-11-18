import opynsim as opyn

def test_color_constructor_behaves_as_expected():
    assert opyn.Color()     == opyn.Color(0.0, 0.0, 0.0, 0.0)
    assert opyn.Color.clear == opyn.Color(0.0, 0.0, 0.0, 0.0)
    assert opyn.Color.white == opyn.Color(1.0, 1.0, 1.0, 1.0)
    assert opyn.Color.black == opyn.Color(0.0, 0.0, 0.0, 1.0)
    assert opyn.Color.red   == opyn.Color(1.0, 0.0, 0.0, 1.0)  # alpha should default to 1.0
    assert opyn.Color.red   == opyn.Color(1.0, 0.0, 0.0)
    assert opyn.Color.green == opyn.Color(0.0, 1.0, 0.0)
    assert opyn.Color.blue  == opyn.Color(0.0, 0.0, 1.0)

    assert opyn.Color.clear != opyn.Color.white
    assert opyn.Color.black != opyn.Color.white
    assert opyn.Color.red   != opyn.Color.green

    assert opyn.Color(b=0.5, r=1.0, g=0.25, a=0.1) == opyn.Color(1.0, 0.25, 0.5, 0.1)

    # And it's hash-able
    assert len({opyn.Color.red, opyn.Color.green, opyn.Color.red}) == 2

def test_color_can_be_stringified():
    assert repr(opyn.Color.green) == "Color(0, 1, 0, 1)"

def test_color_properties_work_as_expected():
    color = opyn.Color.green

    assert color.r == 0.0
    color.r = 0.25
    assert color.r == 0.25

    assert color.g == 1.0
    color.g = 0.5
    assert color.g == 0.5

    assert color.b == 0.0
    color.b = 0.75
    assert color.b == 0.75

    assert color.a == 1.0
    color.a = 0.5
    assert color.a == 0.5
