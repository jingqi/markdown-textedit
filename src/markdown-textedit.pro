
TEMPLATE = subdirs

SUBDIRS += \
    peg-markdown-highlight \
    markdown-textedit \
    test-markdown-textedit

markdown-textedit.depends = peg-markdown-highlight
test-markdown-textedit.depends = markdown-textedit
