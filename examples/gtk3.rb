# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
# see about.rb in this directory for instructions using this.
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

require 'natalie/inline'

__cxx_flags__ "$(pkg-config --cflags gtk+-3.0)"
__ld_flags__ "$(pkg-config --libs gtk+-3.0)"

__inline__ <<-END
  #include <gtk/gtk.h>

  void gtk3_signal_callback(GtkWidget *widget, gpointer data) {
    ProcValue *callback = static_cast<Value*>(data)->as_proc();
    Env *env = callback->env();
    callback->send(env, "call");
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
      Gtk3::widget_set_halign(self, align)
    end

    def set_valign(align)
      Gtk3::widget_set_valign(self, align)
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
      Gtk3::button_new_with_label(label)
    end
  end

  class << self
    def init
      __inline__ "gtk_init(0, nullptr);"
    end

    __define_method__ :window_new, <<-END
      env->assert_argc(argc, 1);
      int type;
      arg_spread(env, argc, args, "i", &type);
      GtkWidget *gtk_window = gtk_window_new((GtkWindowType)type);
      ClassValue *Window = self->const_fetch(env, SymbolValue::intern("Window"))->as_class();
      Value *window_wrapper = new Value { Window };
      Value *ptr = new VoidPValue { env, gtk_window };
      window_wrapper->ivar_set(env, "@_ptr", ptr);
      return window_wrapper;
    END

    __define_method__ :widget_show_all, <<-END
      env->assert_argc(argc, 1);
      GtkWidget *gtk_window;
      arg_spread(env, argc, args, "v", &gtk_window);
      gtk_widget_show_all(gtk_window);
      return NilValue::the();
    END

    __define_method__ :window_set_title, <<-END
      env->assert_argc(argc, 2);
      GtkWidget *gtk_window;
      char *title;
      arg_spread(env, argc, args, "vs", &gtk_window, &title);
      gtk_window_set_title(GTK_WINDOW(gtk_window), title);
      return NilValue::the();
    END

    __define_method__ :box_new, <<-END
      env->assert_argc(argc, 2);
      int orientation, spacing;
      arg_spread(env, argc, args, "ii", &orientation, &spacing);
      GtkWidget *gtk_box = gtk_box_new((GtkOrientation)orientation, spacing);
      ClassValue *Box = self->const_fetch(env, SymbolValue::intern("Box"))->as_class();
      Value *box_wrapper = new Value { Box };
      Value *ptr = new VoidPValue { env, gtk_box };
      box_wrapper->ivar_set(env, "@_ptr", ptr);
      return box_wrapper;
    END

    __define_method__ :container_add, <<-END
      env->assert_argc(argc, 2);
      GtkWidget *gtk_window, *gtk_container;
      arg_spread(env, argc, args, "vv", &gtk_window, &gtk_container);
      gtk_container_add(GTK_CONTAINER(gtk_window), gtk_container);
      return NilValue::the();
    END

    __define_method__ :image_new_from_file, <<-END
      env->assert_argc(argc, 1);
      char *filename;
      arg_spread(env, argc, args, "s", &filename);
      GtkWidget *gtk_image = gtk_image_new_from_file(filename);
      ClassValue *Image = self->const_fetch(env, SymbolValue::intern("Image"))->as_class();
      Value *image_wrapper = new Value { Image };
      Value *ptr = new VoidPValue { env, gtk_image };
      image_wrapper->ivar_set(env, "@_ptr", ptr);
      return image_wrapper;
    END

    __define_method__ :box_pack_start, <<-END
      env->assert_argc(argc, 5);
      GtkWidget *gtk_box, *gtk_child;
      bool expand, fill;
      int padding;
      arg_spread(env, argc, args, "vvbbi", &gtk_box, &gtk_child, &expand, &fill, &padding);
      gtk_box_pack_start(GTK_BOX(gtk_box), gtk_child, expand, fill, padding);
      return NilValue::the();
    END

    __define_method__ :widget_set_halign, <<-END
      env->assert_argc(argc, 2);
      GtkWidget *gtk_widget;
      int align;
      arg_spread(env, argc, args, "vi", &gtk_widget, &align);
      gtk_widget_set_halign(gtk_widget, (GtkAlign)align);
      return NilValue::the();
    END

    __define_method__ :widget_set_valign, <<-END
      env->assert_argc(argc, 2);
      GtkWidget *gtk_widget;
      int align;
      arg_spread(env, argc, args, "vi", &gtk_widget, &align);
      gtk_widget_set_valign(gtk_widget, (GtkAlign)align);
      return NilValue::the();
    END

    __define_method__ :label_new, <<-END
      env->assert_argc(argc, 1);
      GtkWidget *gtk_label;
      if (args[0]->is_nil()) {
          gtk_label = gtk_label_new(nullptr);
      } else {
          const char *text = args[0]->as_string()->c_str();
          gtk_label = gtk_label_new(text);
      }
      ClassValue *Label = self->const_fetch(env, SymbolValue::intern("Label"))->as_class();
      Value *label_wrapper = new Value { Label };
      Value *ptr = new VoidPValue { env, gtk_label };
      label_wrapper->ivar_set(env, "@_ptr", ptr);
      return label_wrapper;
    END

    __define_method__ :label_set_markup, <<-END
      env->assert_argc(argc, 2);
      GtkWidget *gtk_label;
      char *markup;
      arg_spread(env, argc, args, "vs", &gtk_label, &markup);
      gtk_label_set_markup(GTK_LABEL(gtk_label), markup);
      return NilValue::the();
    END

    __define_method__ :button_new_with_label, <<-END
      env->assert_argc(argc, 1);
      char *label;
      arg_spread(env, argc, args, "s", &label);
      GtkWidget *gtk_button = gtk_button_new_with_label(label);
      ClassValue *Button = self->const_fetch(env, SymbolValue::intern("Button"))->as_class();
      Value *button_wrapper = new Value { Button };
      Value *ptr = new VoidPValue { env, gtk_button };
      button_wrapper->ivar_set(env, "@_ptr", ptr);
      return button_wrapper;
    END

    __define_method__ :container_set_border_width, <<-END
      env->assert_argc(argc, 2);
      GtkWidget *container;
      int width;
      arg_spread(env, argc, args, "vi", &container, &width);
      gtk_container_set_border_width(GTK_CONTAINER(container), width);
      return NilValue::the();
    END

    __define_method__ :g_signal_connect, <<-END
      env->assert_argc(argc, 3);
      GObject *instance;
      char *signal;
      Value *callback;
      arg_spread(env, argc, args, "vso", &instance, &signal, &callback);
      g_signal_connect(instance, signal, G_CALLBACK(gtk3_signal_callback), callback);
      return NilValue::the();
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
