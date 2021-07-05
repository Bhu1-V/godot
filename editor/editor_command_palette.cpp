/*************************************************************************/
/*  editor_command_palette.cpp                                           */
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

#include "editor/editor_command_palette.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "scene/gui/tree.h"

EditorCommandPalette *EditorCommandPalette::singleton = nullptr;

float EditorCommandPalette::_score_path(const String &p_search, const String &p_path) {
	float score = 0.9f + .1f * (p_search.length() / (float)p_path.length());

	// Positive bias for matches close to the beginning of the file name.
	int pos = p_path.findn(p_search);
	if (pos != -1) {
		return score * (1.0f - 0.1f * (float(pos) / p_path.length()));
	}

	// Positive bias for matches close to the end of the path.
	pos = p_path.rfindn(p_search);
	if (pos != -1) {
		return score * (0.8f - 0.1f * (float(p_path.length() - pos) / p_path.length()));
	}

	// Remaining results belong to the same class of results.
	return score * 0.69f;
}

void EditorCommandPalette::_update_command_search() {
	commands.get_key_list(&command_keys);
	ERR_FAIL_COND(command_keys.is_empty());


	const String search_text = command_search_box->get_text();
	const bool empty_search = search_text.is_empty();

	// Filter possible candidates.
	Vector<CommandEntry> entries;
	for (int i = 0; i < command_keys.size(); i++) {
		CommandEntry r;
		r.key_name = command_keys[i];
		r.display_name = commands[r.key_name].name;
		if (!empty_search && search_text.is_subsequence_ofi(r.display_name)) {
			r.score = empty_search ? 0 : _score_path(search_text, r.display_name.to_lower());
			entries.push_back(r);
		}
	}

	command_keys.clear();

	TreeItem *root = search_options->get_root();
	root->clear_children();

	if (entries.size() > 0) {
		if (!empty_search) {
			SortArray<CommandEntry, CommandEntryComparator> sorter;
			sorter.sort(entries.ptrw(), entries.size());
		}

		const int entry_limit = MIN(entries.size(), 300);
		for (int i = 0; i < entry_limit; i++) {
			TreeItem *ti = search_options->create_item(root);
			ti->set_tooltip(0, entries[i].key_name);
			ti->set_text(0, entries[i].display_name);
		}

		TreeItem *to_select = root->get_first_child();
		to_select->select(0);
		to_select->set_as_cursor(0);
		search_options->scroll_to_item(to_select);

		get_ok_button()->set_disabled(false);
	} else {
		search_options->deselect_all();

		get_ok_button()->set_disabled(true);
	}
}

void EditorCommandPalette::_bind_methods() {
	ADD_SIGNAL(MethodInfo("execute_command"));
	ADD_SIGNAL(MethodInfo("open_resource"));
}

void EditorCommandPalette::_sbox_input(const Ref<InputEvent> &p_ie) {
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

void EditorCommandPalette::_confirmed() {
	TreeItem *selected_option = search_options->get_selected();
	set_selected_commmad(selected_option->get_tooltip(0));
	emit_signal("execute_command");
	hide();
}

void EditorCommandPalette::set_selected_commmad(String command) {
	selected_command = command;
}

String EditorCommandPalette::get_selected_command() {
	return selected_command;
}

void EditorCommandPalette::open_popup() {
	popup_centered_clamped(Size2i(600, 440), 0.8f);
	command_search_box->clear();
	command_search_box->grab_focus();
}

void EditorCommandPalette::get_actions_list(List<String> *p_list) const {
	commands.get_key_list(p_list);
}

void EditorCommandPalette::add_command(String p_command_name, String p_key_name, Callable p_action, Vector<Variant> arguments) {
	ERR_FAIL_COND_MSG(commands.has(p_key_name), "The EditorAction '" + String(p_command_name) + "' already exists. Unable to add it.");

	const Variant **argptrs = (const Variant **)alloca(sizeof(Variant *) * arguments.size());
	for (int i = 0; i < arguments.size(); i++) {
		argptrs[i] = &arguments[i];
	}
	Command p_command;
	p_command.name = p_command_name;
	p_command.callable = p_action.bind(argptrs, arguments.size());

	commands[p_key_name] = p_command;
}

void EditorCommandPalette::execute_command(String p_command_name) {
	ERR_FAIL_COND_MSG(!commands.has(p_command_name), p_command_name + " not found.");
	commands[p_command_name].callable.call_deferred(nullptr, 0);
}

void EditorCommandPalette::register_shortcuts_as_command() {
	const String *p_key = nullptr;
	p_key = unregisterd_shortcuts.next(p_key);
	while (p_key != nullptr) {
		String command_name = unregisterd_shortcuts[*p_key].first;
		Ref<Shortcut> p_shortcut = unregisterd_shortcuts[*p_key].second;
		Ref<InputEventShortcut> ev;
		ev.instantiate();
		ev->set_shortcut(p_shortcut);
		add_command(command_name, *p_key, callable_mp(EditorNode::get_singleton()->get_viewport(), &Viewport::unhandled_input), varray(ev, false));
		p_key = unregisterd_shortcuts.next(p_key);
	}
	unregisterd_shortcuts.clear();
}

Ref<Shortcut> EditorCommandPalette::add_shortcut_command(const String &p_command, const String &p_key, Ref<Shortcut> p_shortcut) {
	if (is_inside_tree()) {
		Ref<InputEventShortcut> ev;
		ev.instantiate();
		ev->set_shortcut(p_shortcut);
		add_command(p_command, p_key, callable_mp(EditorNode::get_singleton()->get_viewport(), &Viewport::unhandled_input), varray(ev, false));
	} else {
		const String command_name = String(p_command);
		const String key_name = String(p_key);
		Pair p_pair = Pair(command_name, p_shortcut);
		unregisterd_shortcuts[key_name] = p_pair;
	}
	return p_shortcut;
}

EditorCommandPalette *EditorCommandPalette::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(EditorCommandPalette);
	}
	return singleton;
}

EditorCommandPalette::EditorCommandPalette() {
	allow_multi_select = false;

	VBoxContainer *vbc = memnew(VBoxContainer);
	// todo: add theme change signal handler
	add_child(vbc);

	command_search_box = memnew(LineEdit);
	command_search_box->set_placeholder("search for a command");
	command_search_box->set_placeholder_alpha(0.5);
	command_search_box->connect("gui_input", callable_mp(this, &EditorCommandPalette::_sbox_input));
	command_search_box->connect("text_changed", callable_mp(this, &EditorCommandPalette::_update_command_search).unbind(1));
	command_search_box->connect("text_submitted", callable_mp(this, &EditorCommandPalette::_confirmed).unbind(1));
	vbc->add_margin_child(TTR("Search:"), command_search_box);
	register_text_enter(command_search_box);

	search_options = memnew(Tree);
	search_options->connect("item_activated", callable_mp(this, &EditorCommandPalette::_confirmed));
	search_options->create_item();
	search_options->set_hide_root(true);
	search_options->set_hide_folding(true);
	search_options->add_theme_constant_override("draw_guides", 1);
	vbc->add_margin_child(TTR("Matches:"), search_options, true);
}

Ref<Shortcut> ED_SHORTCUT_AND_COMMAND(const String &p_path, const String &p_name, uint32_t p_keycode, String p_command_name) {
	if (p_command_name.is_empty()) {
		p_command_name = p_name;
	}

	Ref<Shortcut> p_shortcut = ED_SHORTCUT(p_path, p_name, p_keycode);
	EditorCommandPalette::get_singleton()->add_shortcut_command(p_command_name, p_path, p_shortcut);
	return p_shortcut;
}
