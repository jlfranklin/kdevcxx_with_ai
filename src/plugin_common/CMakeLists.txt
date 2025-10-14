set(CMAKE_AUTOMOC ON)

find_package(KF6TextEditor REQUIRED)

add_library( plugin_common STATIC )

add_library( kdevcxx_with_ai::plugin_common ALIAS plugin_common )

target_sources(plugin_common PRIVATE
    plugin_common.cc
    info_dialog.cc
    markdown_view.cc
)

target_link_libraries(plugin_common PUBLIC
    ai_processing::core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    KF6::TextEditor
    KF6::I18n
    ${BASIC_LINK_FLAGS}
)
target_compile_options( plugin_common PUBLIC "-fexceptions")

target_include_directories( plugin_common PUBLIC include ${CMAKE_CURRENT_SOURCE_DIR} )


# add_executable( markdown_view EXCLUDE_FROM_ALL)
# target_sources(markdown_view PRIVATE
#   markdown_view.cc
#   markdown_view_test.cc
# )
# target_link_libraries(markdown_view Qt6::Widgets Qt6::WebEngineWidgets)
#
