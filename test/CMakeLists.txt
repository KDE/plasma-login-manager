set(QT_USE_QTTEST TRUE)

include_directories(../src/common)

set(ConfigurationTest_SRCS ConfigurationTest.cpp ../src/common/ConfigReader.cpp)
add_executable(ConfigurationTest ${ConfigurationTest_SRCS})
add_test(NAME Configuration COMMAND ConfigurationTest)

target_link_libraries(ConfigurationTest Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test)

# set(SessionTest_SRCS SessionTest.cpp ../src/common/Configuration.cpp ../src/common/ConfigReader.cpp ../src/common/Session.cpp)
# add_executable(SessionTest ${SessionTest_SRCS})
# target_include_directories(SessionTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../src/common)
# add_test(NAME Session COMMAND SessionTest)
# target_link_libraries(SessionTest Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test)
