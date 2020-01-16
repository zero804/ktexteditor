# This branch

* Changes in this branch are meant for inclusion in KF6
* Changes in this branch concern the APIs for writing plugins and embedding plugins into a host application
* Changes in this branch are _thought to be_ non-controversial. There may not be agreement on the exact details of some (they may not even be technically functional), but there *_is_ agreement on the basic ideas.

# Issues

I'm not perfectly happy with the introduction of KTextEditor::Plugin::PluginView. The problem
is that KXMLGUIClient is not QObject-derived (due to inheritance troubles, I think), and so,
formally requiring plugin views to inherit KXMLGUIClient is harder than expected.
The new struct still allows plugins to provide a KXMLGUIClient or not, and only serves to
encapsulate both the QObject pointer (needed for casting), and the client in a single return statement.

# Dependent applications

There is (going to be) a branch by the same name in kate, and possibly in other repositories.

# Related discussions / resources

* https://mail.kde.org/pipermail/kwrite-devel/2020-January/006002.html (and replies)
* https://mail.kde.org/pipermail/kwrite-devel/2020-January/006034.html (and replies)

