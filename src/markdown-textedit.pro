
TEMPLATE = subdirs

SUBDIRS += \
    markdown-textedit \
    test-markdown-textedit

test-markdown-textedit.depends = markdown-textedit
