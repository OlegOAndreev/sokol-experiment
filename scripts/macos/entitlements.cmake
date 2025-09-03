set(SCRIPTS_MACOS_DIR ${CMAKE_CURRENT_LIST_DIR})

# See https://github.com/cmyr/cargo-instruments/issues/40#issuecomment-894287229
function(target_add_macos_entitlements TARGET)
  if(APPLE)
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND codesign
          -s - -f
          --entitlements ${SCRIPTS_MACOS_DIR}/entitlements.plist
          "$<TARGET_FILE:${TARGET}>"
      VERBATIM
    )
  endif()
endfunction()
