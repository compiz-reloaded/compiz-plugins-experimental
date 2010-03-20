find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (atlantis PLUGINDEPS opengl composite cube)
