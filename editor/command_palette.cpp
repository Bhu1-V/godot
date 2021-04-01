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
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "scene/gui/tree.h"

void CommandPalette::_update_search() {
	print_line(command_search_box->get_text());
}

String CommandPalette::get_command_text() const {
	const String cmd = command_search_box->get_text();
	if (cmd[0] == '>') {
		return cmd.substr(1, -1);
	} else {
		return cmd;
	}
}

void CommandPalette::_update_command_search() {
	List<String> *actions_list = memnew(List<String>);
	EditorNode::get_actions()->get_action_list(actions_list);

	if (actions_list) {
		const String search_text = get_command_text();
		const bool empty_search = search_text == "";

		// Filter possible candidates.
		Vector<String> entries;
		for (int i = 0; i < actions_list->size(); i++) {
			String potential_entry = (*actions_list)[i];
			if (!empty_search && search_text.is_subsequence_ofi(potential_entry)) {
				// r.score = empty_search ? 0 : _score_path(search_text, files[i].to_lower());
				// print_line("found entry : " + (*actions_list)[i]);
				entries.push_back(potential_entry);
			}
		}

		actions_list->clear();

		TreeItem *root = search_options->get_root();
		root->clear_children();

		if (entries.size() > 0) {
			// if (!empty_search) {
			// 	SortArray<Entry, EntryComparator> sorter;
			// 	sorter.sort(entries.ptrw(), entries.size());
			// }

			const int entry_limit = MIN(entries.size(), 300);
			for (int i = 0; i < entry_limit; i++) {
				TreeItem *ti = search_options->create_item(root);
				ti->set_text(0, entries[i]);
				// ti->set_icon(0, *icons.lookup_ptr(entries[i].path.get_extension()));
			}

			TreeItem *to_select = root->get_children();
			to_select->select(0);
			to_select->set_as_cursor(0);
			search_options->scroll_to_item(to_select);

			get_ok_button()->set_disabled(false);
		} else {
			search_options->deselect_all();

			get_ok_button()->set_disabled(true);
		}
	} else {
		print_error("Failed to make action_list.");
	}
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

void CommandPalette::_bind_methods() {
	ADD_SIGNAL(MethodInfo("execute_command"));
	ADD_SIGNAL(MethodInfo("open_resource"));
}

void CommandPalette::_sbox_input(const Ref<InputEvent> &p_ie) {
	Ref<InputEventKey> k = p_ie;
	if (k.is_valid()) {
		switch (k->get_keycode()) {
			case KEY_UP:
			case KEY_DOWN:
			case KEY_PAGEUP:
			case KEY_PAGEDOWN: {
				search_options->call("_gui_input", k);
				command_search_box->accept_event();
			} break;
		}
	}
}

void CommandPalette::_cleanup() {
	print_line("cleaning everything."); //i think this is only for files part but should see.
}

void CommandPalette::set_selected_commmad(String command) {
	selected_command = command;
}

String CommandPalette::get_selected_command() {
	return selected_command;
}

void CommandPalette::_confirmed() {
	TreeItem *selected_option = search_options->get_selected();
	if (!selected_option) {
		return;
	}
	set_selected_commmad(selected_option->get_text(0));
	_cleanup();
	emit_signal("execute_command");
	hide(); // window hide.
}

void CommandPalette::open_popup() {
	popup_centered_clamped(Size2i(600, 440), 0.8f);
	command_search_box->grab_focus();
}

CommandPalette::CommandPalette() {
	allow_multi_select = false;

	VBoxContainer *vbc = memnew(VBoxContainer);
	// todo: add theme change signaal handler
	add_child(vbc);

	command_search_box = memnew(LineEdit);
	command_search_box->set_placeholder("'>' to search command else files");
	command_search_box->set_placeholder_alpha(0.5);
	command_search_box->connect("text_changed", callable_mp(this, &CommandPalette::_text_changed));
	command_search_box->connect("gui_input", callable_mp(this, &CommandPalette::_sbox_input));
	command_search_box->connect("text_entered", callable_mp(this, &CommandPalette::_confirmed));
	vbc->add_child(command_search_box);
	vbc->add_margin_child(TTR("Search:"), command_search_box);
	register_text_enter(command_search_box);

	search_options = memnew(Tree);
	search_options->connect("item_activated", callable_mp(this, &CommandPalette::_confirmed));
	search_options->create_item();
	search_options->set_hide_root(true);
	search_options->set_hide_folding(true);
	search_options->add_theme_constant_override("draw_guides", 1);
	vbc->add_margin_child(TTR("Matches:"), search_options, true);

	_bind_methods();

	// don't need them.
	memdelete(get_ok_button());
	memdelete(get_cancel_button());
	// set_hide_on_ok(false);
}
