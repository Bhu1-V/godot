/*************************************************************************/
/*  code_editor.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/


#include "editor/command_palette.h"

void CommandPalette::_update_search(){
    print_line(command_search_box->get_text());
}

void CommandPalette::_update_command_search(){
    print_line("command : "+command_search_box->get_text());
}

void CommandPalette::_text_changed(const String &p_newtext) {

	if (p_newtext.size()) {
		if (command_search_box->get_text().get(0) == '>') {
			_update_command_search();
		} else {
			_update_search();
		}
	}
}

void CommandPalette::_bind_methods(){
    ADD_SIGNAL(MethodInfo("execute_command"));
    ADD_SIGNAL(MethodInfo("open_resource"));
}

void CommandPalette::_confirmed() {
	print_line("confirmed");
}

void CommandPalette::open_popup() {
    popup_centered_clamped(Size2i(600, 440), 0.8f);
    command_search_box->grab_focus();
    print_line("popup_opened");
}

CommandPalette::CommandPalette() {
    allow_multi_select = false;

    VBoxContainer *vbc = memnew(VBoxContainer);
    // todo: add theme change signaal handler
    add_child(vbc);

    command_search_box = memnew(LineEdit);
    command_search_box->connect("text_changed", callable_mp(this, &CommandPalette::_text_changed));
    // todo: connect gui_input for command_search_box
    vbc->add_margin_child(TTR("Search:"), command_search_box);
    register_text_enter(command_search_box);

    search_options = memnew(Tree);
    search_options->connect("item_activated", callable_mp(this, &CommandPalette::_confirmed));
    search_options->create_item();
	search_options->set_hide_root(true);
	search_options->set_hide_folding(true);
	search_options->add_theme_constant_override("draw_guides", 1);
	vbc->add_margin_child(TTR("Matches:"), search_options, true);

    // don't need them.
    memdelete(get_ok_button());
    memdelete(get_cancel_button());

}
