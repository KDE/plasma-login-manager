

A small binary that shows a plasma wallpaper on every screen
The app that shows the prompt and user-selection stuff on a transparent background

# Prompt

This is a placeholder using greetd + some code from QtGreetD.

I'm not very convinced this is the exact path we want. 
  Greetd misses core features
  and these Qt bindings are not great - they're blocking.

We can continue to use SDDM as a base, but I am convinced we need to make that stripped down to have the same architecture of starting a sesssion and having some fixed IPC instead of the current state where it tries to do half the things for us. 

# This repo contains


# My config.tml

command = "/opt/kde6/lib/libexec/plasma-dbus-run-session-if-needed /opt/kde6/lib/libexec/startplasma-dev.sh -login-wayland"
