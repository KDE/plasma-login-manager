add_library(settings STATIC
    wallpaper_integration.cpp
)

set_property(TARGET settings PROPERTY POSITION_INDEPENDENT_CODE ON)

kconfig_add_kcfg_files(settings GENERATE_MOC kscreensaversettingsbase.kcfgc)
target_link_libraries(settings
    Qt6::Quick
    KF6::ConfigQml
    KF6::Package
)
