# This is just a simple about dialog for Natalie that uses the
# gtk3 library in this same directory.
#
# bin/natalie examples/about.rb

require_relative './gtk3'

destroy = -> { Gtk3.main_quit }

Gtk3.init
window = Gtk3::Window.new(Gtk3::WINDOW_TOPLEVEL)
window.set_title('About Natalie')
window.set_border_width(10)

box1 = Gtk3::Box.new(Gtk3::ORIENTATION_HORIZONTAL, 20)
window.container_add(box1)

icon = Gtk3::Image.new_from_file(File.expand_path('./icon.png', __dir__))
icon.set_halign(Gtk3::ALIGN_START)
icon.set_valign(Gtk3::ALIGN_START)
box1.pack_start(icon, false, false, 0)

box2 = Gtk3::Box.new(Gtk3::ORIENTATION_VERTICAL, 10)
box1.pack_start(box2, true, true, 0)

title = Gtk3::Label.new
title.set_markup('<b>Natalie</b>')
title.set_halign(Gtk3::ALIGN_START)
title.set_valign(Gtk3::ALIGN_START)
box2.pack_start(title, false, false, 0)

body = Gtk3::Label.new(<<-END)
Compiled Ruby Implementation
targeting Ruby 2.7 (WIP)

Copyright © 2019-2020, Tim Morgan



This product is licensed to:
Everybody! :^)
END
body.set_halign(Gtk3::ALIGN_START)
body.set_valign(Gtk3::ALIGN_START)
box2.pack_start(body, false, false, 0)

button = Gtk3::Button.new_with_label('OK')
button.set_halign(Gtk3::ALIGN_END)
button.signal_connect('clicked', destroy)
box2.pack_start(button, false, false, 0)

Gtk3.widget_show_all(window)

window.signal_connect('destroy', destroy)

Gtk3.main
