macro(prefix_postfix_list_items InputList PrefixString PostfixString OutputName)
    if("${InputList}" STREQUAL "${OutputName}")
        message(FATAL_ERROR "Input list name ('${InputList}') cannot be the same as the output list name ('${OutputName}').")
    endif()
	
    set(${OutputName})
	
    foreach(item ${${InputList}})
        list(APPEND ${OutputName} "${PrefixString}${item}${PostfixString}")
    endforeach()
endmacro()