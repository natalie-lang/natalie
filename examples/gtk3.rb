# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# see about.rb in this directory for instructions using this.

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

require 'natalie/inline'

__cxx_flags__ '$(pkg-config --cflags gtk+-3.0)'
__ld_flags__ '$(pkg-config --libs gtk+-3.0)'

__inline__ <<-END
  #include <gtk/gtk.h>

  void gtk3_signal_callback(GtkWidget *widget, gpointer data) {
    ProcObject *callback = static_cast<Object*>(data)->as_proc();
    Env *env = callback->env();
    callback->send(env, "call"_s);
  }
END

module Gtk3
  WINDOW_TOPLEVEL = 0

  ORIENTATION_HORIZONTAL = 0
  ORIENTATION_VERTICAL = 1

  ALIGN_START = 1
  ALIGN_END = 2

  class Widget
    def set_halign(align)
      Gtk3.widget_set_halign(self, align)
    end

    def set_valign(align)
      Gtk3.widget_set_valign(self, align)
    end

    def signal_connect(signal, callback)
      Gtk3.g_signal_connect(self, signal, callback)
    end
  end

  class Window < Widget
    def self.new(type)
      Gtk3.window_new(type)
    end

    def set_title(title)
      Gtk3.window_set_title(self, title)
    end

    def container_add(container)
      Gtk3.container_add(self, container)
    end

    def set_border_width(width)
      Gtk3.container_set_border_width(self, width)
    end
  end

  class Box < Widget
    def self.new(orientation, spacing)
      Gtk3.box_new(orientation, spacing)
    end

    def pack_start(child, expand, fill, padding)
      Gtk3.box_pack_start(self, child, expand, fill, padding)
    end
  end

  class Image < Widget
    def self.new_from_file(filename)
      Gtk3.image_new_from_file(filename)
    end
  end

  class Label < Widget
    def self.new(text = nil)
      Gtk3.label_new(text)
    end

    def set_markup(markup)
      Gtk3.label_set_markup(self, markup)
    end
  end

  class Button < Widget
    def self.new_with_label(label)
      Gtk3.button_new_with_label(label)
    end
  end

  class << self
    def init
      __inline__ 'gtk_init(0, nullptr);'
    end

    __define_method__ :window_new, <<-END
      env->ensure_argc_is(argc, 1);
      int type;
      arg_spread(env, argc, args, "i", &type);
      GtkWidget *gtk_window = gtk_window_new((GtkWindowType)type);
      ClassObject *Window = self->const_fetch("Window"_s)->as_class();
      Object *window_wrapper = new Object { Window };
      Object *ptr = new VoidPObject { gtk_window };
      window_wrapper->ivar_set(env, "@_ptr"_s, ptr);
      return window_wrapper;
    END

    __define_method__ :widget_show_all, <<-END
      env->ensure_argc_is(argc, 1);
      GtkWidget *gtk_window;
      arg_spread(env, argc, args, "v", &gtk_window);
      gtk_widget_show_all(gtk_window);
      return NilObject::the();
    END

    __define_method__ :window_set_title, <<-END
      env->ensure_argc_is(argc, 2);
      GtkWidget *gtk_window;
      char *title;
      arg_spread(env, argc, args, "vs", &gtk_window, &title);
      gtk_window_set_title(GTK_WINDOW(gtk_window), title);
      return NilObject::the();
    END

    __define_method__ :box_new, <<-END
      env->ensure_argc_is(argc, 2);
      int orientation, spacing;
      arg_spread(env, argc, args, "ii", &orientation, &spacing);
      GtkWidget *gtk_box = gtk_box_new((GtkOrientation)orientation, spacing);
      ClassObject *Box = self->const_fetch("Box"_s)->as_class();
      Object *box_wrapper = new Object { Box };
      Object *ptr = new VoidPObject { gtk_box };
      box_wrapper->ivar_set(env, "@_ptr"_s, ptr);
      return box_wrapper;
    END

    __define_method__ :container_add, <<-END
      env->ensure_argc_is(argc, 2);
      GtkWidget *gtk_window, *gtk_container;
      arg_spread(env, argc, args, "vv", &gtk_window, &gtk_container);
      gtk_container_add(GTK_CONTAINER(gtk_window), gtk_container);
      return NilObject::the();
    END

    __define_method__ :image_new_from_file, <<-END
      env->ensure_argc_is(argc, 1);
      char *filename;
      arg_spread(env, argc, args, "s", &filename);
      GtkWidget *gtk_image = gtk_image_new_from_file(filename);
      ClassObject *Image = self->const_fetch("Image"_s)->as_class();
      Object *image_wrapper = new Object { Image };
      Object *ptr = new VoidPObject { gtk_image };
      image_wrapper->ivar_set(env, "@_ptr"_s, ptr);
      return image_wrapper;
    END

    __define_method__ :box_pack_start, <<-END
      env->ensure_argc_is(argc, 5);
      GtkWidget *gtk_box, *gtk_child;
      bool expand, fill;
      int padding;
      arg_spread(env, argc, args, "vvbbi", &gtk_box, &gtk_child, &expand, &fill, &padding);
      gtk_box_pack_start(GTK_BOX(gtk_box), gtk_child, expand, fill, padding);
      return NilObject::the();
    END

    __define_method__ :widget_set_halign, <<-END
      env->ensure_argc_is(argc, 2);
      GtkWidget *gtk_widget;
      int align;
      arg_spread(env, argc, args, "vi", &gtk_widget, &align);
      gtk_widget_set_halign(gtk_widget, (GtkAlign)align);
      return NilObject::the();
    END

    __define_method__ :widget_set_valign, <<-END
      env->ensure_argc_is(argc, 2);
      GtkWidget *gtk_widget;
      int align;
      arg_spread(env, argc, args, "vi", &gtk_widget, &align);
      gtk_widget_set_valign(gtk_widget, (GtkAlign)align);
      return NilObject::the();
    END

    __define_method__ :label_new, <<-END
      env->ensure_argc_is(argc, 1);
      GtkWidget *gtk_label;
      if (args[0]->is_nil()) {
          gtk_label = gtk_label_new(nullptr);
      } else {
          const char *text = args[0]->as_string()->c_str();
          gtk_label = gtk_label_new(text);
      }
      ClassObject *Label = self->const_fetch("Label"_s)->as_class();
      Object *label_wrapper = new Object { Label };
      Object *ptr = new VoidPObject { gtk_label };
      label_wrapper->ivar_set(env, "@_ptr"_s, ptr);
      return label_wrapper;
    END

    __define_method__ :label_set_markup, <<-END
      env->ensure_argc_is(argc, 2);
      GtkWidget *gtk_label;
      char *markup;
      arg_spread(env, argc, args, "vs", &gtk_label, &markup);
      gtk_label_set_markup(GTK_LABEL(gtk_label), markup);
      return NilObject::the();
    END

    __define_method__ :button_new_with_label, <<-END
      env->ensure_argc_is(argc, 1);
      char *label;
      arg_spread(env, argc, args, "s", &label);
      GtkWidget *gtk_button = gtk_button_new_with_label(label);
      ClassObject *Button = self->const_fetch("Button"_s)->as_class();
      Object *button_wrapper = new Object { Button };
      Object *ptr = new VoidPObject { gtk_button };
      button_wrapper->ivar_set(env, "@_ptr"_s, ptr);
      return button_wrapper;
    END

    __define_method__ :container_set_border_width, <<-END
      env->ensure_argc_is(argc, 2);
      GtkWidget *container;
      int width;
      arg_spread(env, argc, args, "vi", &container, &width);
      gtk_container_set_border_width(GTK_CONTAINER(container), width);
      return NilObject::the();
    END

    __define_method__ :g_signal_connect, <<-END
      env->ensure_argc_is(argc, 3);
      GObject *instance;
      char *signal;
      Object *callback;
      arg_spread(env, argc, args, "vso", &instance, &signal, &callback);
      g_signal_connect(instance, signal, G_CALLBACK(gtk3_signal_callback), callback);
      return NilObject::the();
    END

    def main
      __inline__ <<-END
        gtk_main();
      END
    end

    def main_quit
      __inline__ <<-END
        gtk_main_quit();
      END
    end
  end
end
