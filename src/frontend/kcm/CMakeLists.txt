kcmutils_add_qml_kcm(kcm_plasmalogin)
add_definitions(-DTRANSLATION_DOMAIN="kcm_plasmalogin")

target_sources(kcm_plasmalogin PRIVATE
    kcm.cpp kcm.h
    plasmalogindata.cpp plasmalogindata.h
)

target_link_libraries(kcm_plasmalogin PRIVATE
    settings
    KF6::I18n
    KF6::AuthCore
    #KF6::KCMUtils
    KF6::KCMUtilsCore
    KF6::KCMUtilsQuick
    KF6::KIOGui
    KF6::Service
)

add_subdirectory(auth)

ki18n_install(po)
