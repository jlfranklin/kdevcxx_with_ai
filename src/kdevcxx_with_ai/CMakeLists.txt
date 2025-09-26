set(kdevcxx_with_ai_SRCS
    kdevcxx_with_ai.cc
    kdevcxx_with_ai_config_page.cc
)

kdevplatform_add_plugin(kdevcxx_with_ai_p
    SOURCES ${kdevcxx_with_ai_SRCS}
)

target_link_libraries(kdevcxx_with_ai_p
    kdevcxx_with_ai::plugin_common
    KF6::CoreAddons
    KF6::I18n
    KF6::TextEditor
    KDev::Interfaces
    ${BASIC_LINK_FLAGS}
)
target_compile_options( kdevcxx_with_ai_p PRIVATE "-fexceptions")
set_target_properties(kdevcxx_with_ai_p PROPERTIES OUTPUT_NAME "kdevcxx_with_ai")

target_compile_definitions(kdevcxx_with_ai_p PRIVATE TRANSLATION_DOMAIN="kdevcxx_with_ai")


install(TARGETS kdevcxx_with_ai_p DESTINATION ${KDE_INSTALL_PLUGINDIR})

message(STATUS "PLUGIN_INSTALL_DIR ${KDE_INSTALL_PLUGINDIR}")
