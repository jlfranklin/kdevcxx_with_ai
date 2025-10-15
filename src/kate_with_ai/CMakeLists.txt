
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

kcoreaddons_add_plugin(kate_with_ai
    INSTALL_NAMESPACE "kf6/ktexteditor"
)

target_sources(kate_with_ai PRIVATE
    kate_with_ai.cc
    kate_with_ai_config_page.cc
    kate_with_ai.json
)

target_compile_definitions(kate_with_ai PRIVATE TRANSLATION_DOMAIN="kate_with_ai")

target_link_libraries(kate_with_ai PRIVATE
  kdevcxx_with_ai::plugin_common
  ${BASIC_LINK_FLAGS}
  KF6::CoreAddons
  KF6::I18n
  KF6::TextEditor
)
