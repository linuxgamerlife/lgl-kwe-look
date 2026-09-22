# QtTest suite. Every test links the core library and runs on the offscreen platform so no
# display is needed. Each test points HOME and the XDG variables at a temporary directory.
function(lglkwe_add_test name)
    add_executable(${name} ${name}.cpp testenv.h)
    target_link_libraries(${name} PRIVATE lglkwe-core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

lglkwe_add_test(tst_inifile)
lglkwe_add_test(tst_gsettings)
lglkwe_add_test(tst_themescan)
lglkwe_add_test(tst_xcursor)
lglkwe_add_test(tst_gtkexport)
lglkwe_add_test(tst_qtct)
lglkwe_add_test(tst_sinks)
lglkwe_add_test(tst_migration)
