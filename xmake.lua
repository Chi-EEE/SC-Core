add_rules("mode.debug", "mode.release")
set_languages("cxx17")

add_requires("cpptrace")

target("supercell_core")
    set_kind("$(kind)")
    add_packages("cpptrace")

    add_files("source/**.cpp")
    add_headerfiles("include/(**.h)")
    add_includedirs("include")
