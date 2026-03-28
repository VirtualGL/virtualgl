file(GLOB SOURCE_FILES *.gif *.png *.rb [a-z]*.txt virtualgl.css)

file(GLOB DOC_FILES *.gif *.png LICENSE*.txt LGPL.txt index.html virtualgl.css)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stamp
	COMMAND deplate -c config.rb -m html-obfuscate-email virtualgl.txt
		-D _DOCVERSION=${DOCVERSION} -D _VERSION=${VERSION} -o index.html
	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/stamp
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${SOURCE_FILES}
	COMMENT "Generating User's Guide"
)

add_custom_target(doc DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stamp)

install(FILES ${DOC_FILES} ${CMAKE_SOURCE_DIR}/ChangeLog.md
	DESTINATION ${CMAKE_INSTALL_DOCDIR})
