
TEMPLATE = subdirs

SUBDIRS += \
    peg-markdown-highlight \
    pmh-adapter \
    markdown-textedit \
    test-markdown-textedit

markdown-textedit.depends = pmh-adapter peg-markdown-highlight
test-markdown-textedit.depends = markdown-textedit
