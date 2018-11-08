from docutils import nodes
from docutils.parsers import rst
from sphinx.domains import Domain
import os.path

# This is a place to hang git file references.
class SWOCDomain(Domain):
    """
    Solid Wall Of Code.
    """

    name = 'swoc'
    label = 'SWOC'
    data_version = 1

def make_github_link(name, rawtext, text, lineno, inliner, options={}, content=[]):
    """
    This docutils role lets us link to source code via the handy :swoc:git: markup.
    """
    url = 'https://github.com/SolidWallOfCode/libswoc/blob/{}/{}'
    ref = 'master'
    node = nodes.reference(rawtext, os.path.basename(text), refuri=url.format(ref, text), **options)
    return [node], []

def setup(app):
    rst.roles.register_generic_role('arg', nodes.emphasis)
    rst.roles.register_generic_role('const', nodes.literal)
    rst.roles.register_generic_role('pack', nodes.strong)

    app.add_domain(SWOCDomain)

    # this lets us do :swoc:git:`<file_path>` and link to the file on github
    app.add_role_to_domain('swoc', 'git', make_github_link)
