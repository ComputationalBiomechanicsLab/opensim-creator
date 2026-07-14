# see: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool
# and: https://www.files.certum.eu/documents/manual_en/Code-Signing-signing-the-code-using-tools-like-Singtool-and-Jarsigner_v2.3.pdf
set(CPACK_OSC_CERTIFICATE_SUBJECT          "Open Source Developer, Adam Kewley")  # The name of the subject of the signing certificate
set(CPACK_OSC_CERTIFICATE_TIMESTAMP_SERVER "http://time.certum.pl"             )  # Specifies the URL of the RFC 3161 time stamp serve
set(CPACK_OSC_CERTIFICATE_TIMESTAMP_DIGEST "SHA256"                            )  # Digest algorithm used by the RFC 3161 time stamp server (e.g. SHA256)
set(CPACK_OSC_CODESIGN_DIGEST_ALGORITHM    "SHA256"                            )  # File digest algorithm to use for creating file signature (e.g. SHA256)

# Specify a script that signs the built binaries (`exe`s, `dll`s) just before CPack creates the installer.
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/win_codesign_binaries.cmake")

# Specify a script that signs the installer that CPack builds.
set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/win_codesign_installer.cmake")
