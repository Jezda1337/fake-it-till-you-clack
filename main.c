#include <dirent.h>
#include <fcntl.h>
#include <libudev.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_KEY_CODE 256

const char *key_sounds[MAX_KEY_CODE] = {NULL};

void init_key_sounds() {
  key_sounds[KEY_1] = "q.wav";
  key_sounds[KEY_2] = "w.wav";
  key_sounds[KEY_3] = "e.wav";
  key_sounds[KEY_4] = "e.wav";
  key_sounds[KEY_5] = "r.wav";
  key_sounds[KEY_6] = "t.wav";
  key_sounds[KEY_7] = "y.wav";
  key_sounds[KEY_8] = "u.wav";
  key_sounds[KEY_9] = "i.wav";
  key_sounds[KEY_0] = "o.wav";
  key_sounds[KEY_MINUS] = "p.wav";
  key_sounds[KEY_EQUAL] = "[.wav";

  key_sounds[KEY_LEFTCTRL] = "backspace.wav";
  key_sounds[KEY_RIGHTCTRL] = "backspace.wav";
  key_sounds[KEY_LEFTALT] = "e.wav";
  key_sounds[KEY_RIGHTALT] = "p.wav";

  key_sounds[KEY_SEMICOLON] = "[.wav";
  key_sounds[KEY_APOSTROPHE] = "].wav";
  key_sounds[KEY_COMMA] = "m.wav";
  key_sounds[KEY_DOT] = "[.wav";
  key_sounds[KEY_SLASH] = "].wav";
  key_sounds[KEY_GRAVE] = "q.wav";

  key_sounds[KEY_BACKSPACE] = "backspace.wav";
  key_sounds[KEY_TAB] = "tab.wav";

  key_sounds[KEY_Q] = "q.wav";
  key_sounds[KEY_W] = "w.wav";
  key_sounds[KEY_E] = "e.wav";
  key_sounds[KEY_R] = "r.wav";
  key_sounds[KEY_T] = "t.wav";
  key_sounds[KEY_Y] = "y.wav";
  key_sounds[KEY_U] = "u.wav";
  key_sounds[KEY_I] = "i.wav";
  key_sounds[KEY_O] = "o.wav";
  key_sounds[KEY_P] = "p.wav";
  key_sounds[KEY_LEFTBRACE] = "[.wav";
  key_sounds[KEY_RIGHTBRACE] = "].wav";

  key_sounds[KEY_ENTER] = "enter.wav";

  key_sounds[KEY_A] = "a.wav";
  key_sounds[KEY_S] = "s.wav";
  key_sounds[KEY_D] = "d.wav";
  key_sounds[KEY_F] = "f.wav";
  key_sounds[KEY_G] = "g.wav";
  key_sounds[KEY_H] = "h.wav";
  key_sounds[KEY_J] = "j.wav";
  key_sounds[KEY_K] = "k.wav";
  key_sounds[KEY_L] = "l.wav";
  key_sounds[KEY_LEFTBRACE] = "[.wav";
  key_sounds[KEY_RIGHTBRACE] = "].wav";

  key_sounds[KEY_Q] = "q.wav";
  key_sounds[KEY_LEFTSHIFT] = "shift.wav";
  key_sounds[KEY_BACKSPACE] = "backspace.wav";

  key_sounds[KEY_Z] = "z.wav";
  key_sounds[KEY_X] = "x.wav";
  key_sounds[KEY_C] = "c.wav";
  key_sounds[KEY_V] = "v.wav";
  key_sounds[KEY_B] = "b.wav";
  key_sounds[KEY_N] = "n.wav";
  key_sounds[KEY_M] = "m.wav";
  key_sounds[KEY_L] = "l.wav";
  key_sounds[KEY_LEFTBRACE] = "[.wav";
  key_sounds[KEY_RIGHTBRACE] = "].wav";
  key_sounds[KEY_RIGHTSHIFT] = "shift.wav";

  key_sounds[KEY_UP] = "g.wav";
  key_sounds[KEY_LEFT] = "c.wav";
  key_sounds[KEY_RIGHT] = "b.wav";
  key_sounds[KEY_DOWN] = "v.wav";

  key_sounds[KEY_O] = "o.wav";
  key_sounds[KEY_E] = "e.wav";
  key_sounds[KEY_SPACE] = "space.wav";
  key_sounds[KEY_CAPSLOCK] = "caps lock.wav";
}

int find_keyboard_device() {
  struct udev *udev = udev_new();
  if (!udev) {
    fprintf(stderr, "Failed to create udev\n");
    return -1;
  }

  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_add_match_property(enumerate, "ID_INPUT_KEYBOARD", "1");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *dev_list_entry;

  int fd = -1;
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path = udev_list_entry_get_name(dev_list_entry);
    struct udev_device *dev = udev_device_new_from_syspath(udev, path);

    struct udev_device *pdev =
        udev_device_get_parent_with_subsystem_devtype(dev, "input", NULL);
    const char *devnode = udev_device_get_devnode(dev);

    if (devnode) {
      fd = open(devnode, O_RDONLY);
      if (fd >= 0) {
        char name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        printf("Found keyboard: %s (device: %s)\n", name, devnode);
        udev_device_unref(dev);
        break;
      }
    }
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);
  return fd;
}

int running = 1;
int kbd_fd;

void handle_sigint(int sig) {
  running = 0;
  printf("%d", sig);
  printf("\nExiting...\n");
  if (kbd_fd >= 0)
    close(kbd_fd);
  exit(0);
}

int main() {
  init_key_sounds();

  struct input_event ev;

  if (geteuid() == 0) {
    setenv("XDG_RUNTIME_DIR", "/run/user/1000", 1);
    setenv("PULSE_SERVER", "unix:/run/user/1000/pulse/native", 1);
  }
  signal(SIGINT, handle_sigint);

  kbd_fd = find_keyboard_device();
  if (kbd_fd < 0) {
    fprintf(stderr, "Could not find keyboard device\n");
    return 1;
  }

  printf("Listening for key events. Press Ctrl+C to exit.\n");

  int ctrl_pressed = 0;

  while (running) {
    ssize_t n = read(kbd_fd, &ev, sizeof(ev));
    if (n == sizeof(ev) && ev.type == EV_KEY) {
      if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL) {
        ctrl_pressed = ev.value;
      }
      if (ctrl_pressed && ev.code == KEY_C && ev.value == 1) {
        handle_sigint(SIGINT);
      }
      if (ev.value == 1) {
        const char *sound = NULL;
        if (ev.code < MAX_KEY_CODE)
          sound = key_sounds[ev.code];
        if (sound) {
          char cmd[512];
          snprintf(cmd, sizeof(cmd), "paplay './assets/%s' &", sound);
          system(cmd);
        }
      }
    }
  }

  close(kbd_fd);
  return 0;
}
