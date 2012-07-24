if (UNIX)
	ADD_CUSTOM_TARGET(
		prepare-docs 
		doxygen 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
		COMMENT generate the documentation in the source directory using \"make docs\" - doxygen is required)
		set_target_properties(prepare-docs PROPERTIES FOLDER "Documentation")

	ADD_CUSTOM_TARGET(
	  docs
		bash -c "find . -name *.png -exec convert {} -colors 256 -depth 16 {} \;"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Decreasing quality of images in documentation"
		VERBATIM
	)
	add_dependencies(docs prepare-docs)
else()
	ADD_CUSTOM_TARGET(
		docs 
		doxygen 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
		COMMENT generate the documentation in the source directory using \"make docs\" - doxygen is required)

endif()
set_target_properties(docs PROPERTIES FOLDER "Documentation")
