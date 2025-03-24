# Due to copy pasted code from QtGreet
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY=0)
remove_definitions(-DQT_USE_QSTRINGBUILDER)

set(LOGIN_GREETER_SRCS main.cpp
    backend/GreeterProxy.cpp
    backend/SocketWriter.cpp
    mockbackend/MockGreeterProxy.cpp
)

ecm_qt_declare_logging_category(LOGIN_GREETER_SRCS HEADER debug.h
                                IDENTIFIER LOGOUT_GREETER
                                CATEGORY_NAME kde.login_greeter
                                DEFAULT_SEVERITY Debug
                                DESCRIPTION "login greeter debug"
                                EXPORT PLASMAWORKSPACE
                            )

add_executable(plasma-login-greeter ${LOGIN_GREETER_SRCS})

ecm_add_qml_module(plasma-login-greeter URI "org.kde.plasma.login" GENERATE_PLUGIN_SOURCE)
ecm_target_qml_sources(plasma-login-greeter
    SOURCES
    qml/Main.qml
    qml/Login.qml
    qml/SessionButton.qml
    qml/KeyboardButton.qml

    RESOURCES
    qml/faces/.face.icon
)

target_link_libraries(plasma-login-greeter PRIVATE
    settings
    Qt::Quick
    Qt::DBus
    KF6::I18n
    Plasma::PlasmaQuick
    KF6::Package
    KF6::WindowSystem
    KF6::ConfigCore
    X11::X11
    LayerShellQt::Interface
    PW::KWorkspace
)

ecm_finalize_qml_module(plasma-login-greeter)

install(TARGETS plasma-login-greeter DESTINATION ${KDE_INSTALL_LIBEXECDIR})

ecm_install_configured_files(INPUT plasma-login.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
