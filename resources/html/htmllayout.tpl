{% msg %}----- Start generating HTML output for from template ----{% endmsg %}

{# ---- copy fixed resources to the output ----- #}

{% resource 'doxygen.css' %}
{% resource 'tabs.css' %}
{% resource 'jquery.js' %}
{% resource 'dynsections.js %}
{% resource 'tab_a.lum' %}
{% resource 'tab_b.lum' %}
{% resource 'tab_h.lum' %}
{% resource 'tab_s.lum' %}
{% resource 'tab_h.lum' %}
{% resource 'bc_s.luma' %}
{% resource 'doxygen.luma' %}
{% resource 'closed.luma' %}
{% resource 'open.luma' %}
{% resource 'bdwn.luma' %}
{% resource 'sync_on.luma' %}
{% resource 'sync_off.luma' %}

{# navigation #}
{% resource 'nav_f.lum' %}
{% resource 'nav_g.png' %}
{% resource 'nav_h.lum' %}
{% resource 'navtree.css' %}

{# general search resources #}
{% resource 'search_l.png' as 'search/search_l.png' %}
{% resource 'search_m.png' as 'search/search_m.png' %}
{% resource 'search_r.png' as 'search/search_r.png' %}
{% if config.DISABLE_INDEX %}
  {% resource 'search_noidx.css' as 'search/search.css'   %}
{% else %}
  {% resource 'search.css'       as 'search/search.css'   %}
{% endif %}

{% if config.SERVER_BASED_SEARCH %}
  {# server side search resources #}
  {% resource 'mag.png'     as 'search/mag.png'     %}
  {% resource 'extsearch.js as 'search/search.js'   %}
  {% resource 'search_functions.php'  as 'search/search_functions.php'  %}
  {% resource 'search_opensearch.php' as 'search/search_opensearch.php' %}
{% else %}
  {# client side search resources #}
  {% resource 'mag_sel.png' as 'search/mag_sel.png' %}
  {% resource 'close.png'   as 'search/close.png'   %}
  {% resource 'search.js'   as 'search/search.js'   %}
{% endif %}

{# interactive SVGs #}
{% resource 'svgpan.js' %}

{# -------------------------------------------------- #}

{# global constants #}
{% set maxItemsForFlatList=2 %}
{% set maxItemsForMultiPageList=4 %}

{# global variable #}
{% set page_postfix='' %}

{# open the global navigation index #}
{% indexentry nav name=tr.mainPage file='index' anchor='' %}
{% opensubindex nav %}

{# ----------- HTML DOCUMENTATION PAGES ------------ #}

{# write main page documentation #}
{% if mainPage %}
  {% with page=mainPage compound=mainPage %}
    {% create mainPage.fileName|append:config.HTML_FILE_EXTENSION from 'htmlpage.tpl' %}
  {% endwith %}
{% endif %}

{# write namespace documentation pages #}
{% for compound in namespaceList %}
  {% with page=compound %}
    {% create compound.fileName|append:config.HTML_FILE_EXTENSION from 'htmlnamespace.tpl' %}
  {% endwith %}
{% endfor %}

{# write class documentation pages #}
{% for compound in classList %}
  {% with page=compound %}
    {% create compound.fileName|append:config.HTML_FILE_EXTENSION from 'htmlclass.tpl' %}
    {% if compound.allMembersList and not config.OPTIMIZE_OUTPUT_FOR_C %}
      {% create compound.allMembersFileName|append:config.HTML_FILE_EXTENSION from 'htmlallmembers.tpl' %}
    {% endif %}
  {% endwith %}
{% endfor %}

{# write the file sources #}
{% for compound in fileList %}
  {% with page=compound %}
    {# TODO: to deal with clang optimisation, we need to write the sources in a different order! #}
    {# TODO: now writing sources has the side-effect of creating cross-references. Need to split that up! #}
    {% if compound.hasSourceFile %}
      {% create compound.sourceFileName|append:config.HTML_FILE_EXTENSION from 'htmlsource.tpl' %}
    {% endif %}
  {% endwith %}
{% endfor %}

{# write file documentation pages #}
{% for compound in fileList %}
  {% with page=compound %}
    {% create compound.fileName|append:config.HTML_FILE_EXTENSION from 'htmlfile.tpl' %}
  {% endwith %}
{% endfor %}

{# write related page documentation #}
{% for compound in pageList %}
  {% with page=compound %}
    {% create compound.fileName|append:config.HTML_FILE_EXTENSION from 'htmlpage.tpl' %}
  {% endwith %}
{% endfor %}

{# write module documentation #}
{% for compound in moduleList %}
  {% with page=compound %}
    {% create compound.fileName|append:config.HTML_FILE_EXTENSION from 'htmlmodule.tpl' %}
  {% endwith %}
{% endfor %}

{# ----------- INDEXES ------------ #}

{# --- related pages --- #}
{% if pageTree.tree %}
  {% with page=pageTree %}
    {% create pageTree.fileName|append:config.HTML_FILE_EXTENSION from 'htmlpages.tpl' %}
  {% endwith %}
{% endif %}

{# --- modules --- #}
{% if moduleTree.tree %}
  {% with page=moduleTree %}
    {% create moduleTree.fileName|append:config.HTML_FILE_EXTENSION from 'htmlmodules.tpl' %}
  {% endwith %}
{% endif %}

{# --- namespaces --- #}
{% indexentry nav name=tr.namespaces file='' anchor='' %}
{% opensubindex nav %}

  {% if namespaceTree.tree %}
    {% with page=namespaceTree %}
      {% create namespaceTree.fileName|append:config.HTML_FILE_EXTENSION from 'htmlnamespaces.tpl' %}
    {% endwith %}
  {% endif %}

  {# write symbol indices for namespace members #}
  {% if namespaceMembersIndex.all %}
    {% with page=namespaceMembersIndex scope='namespace' template='htmlnsmembers.tpl' %}
      {% indexentry nav name=tr.namespaceMembers file=page.fileName anchor='' %}
      {% include 'htmlmembersindex.tpl' %}
    {% endwith %}
  {% endif %}

{% closesubindex nav %}

{# --- classes --- #}
{% indexentry nav name=tr.classes file='' anchor='' %}
{% opensubindex nav %}

  {# write the annotated class list #}
  {% if classTree.tree %}
    {% with page=classTree %}
      {% create classTree.fileName|append:config.HTML_FILE_EXTENSION from 'htmlannotated.tpl' %}
    {% endwith %}
  {% endif %}

  {# write class index #}
  {% if classIndex.list %}
    {% with page=classIndex %}
      {% create classIndex.fileName|append:config.HTML_FILE_EXTENSION from 'htmlclasses.tpl' %}
    {% endwith %}
  {% endif %}

  {# TODO: write the class inheritance hierarchy #}
  {% if classHierarchy.tree %}
    {% with page=classHierarchy %}
      {% create classHierarchy.fileName|append:config.HTML_FILE_EXTENSION from 'hierarchy.tpl' %}
    {% endwith %}
  {% endif %}

  {# write symbol indices for class members #}
  {% if classMembersIndex.all %}
    {% with page=classMembersIndex scope='class' template='htmlclmembers.tpl' %}
      {% indexentry nav name=tr.classMembers file=page.fileName anchor='' %}
      {% include 'htmlmembersindex.tpl' %}
    {% endwith %}
  {% endif %}

{% closesubindex nav %}

{# --- files --- #}
{% indexentry nav name=tr.files file='' anchor='' %}
{% opensubindex nav %}

  {# write the directory/file hierarchy #}
  {% if fileTree.tree %}
    {% with page=fileTree %}
      {% create fileTree.fileName|append:config.HTML_FILE_EXTENSION from 'htmlfiles.tpl' %}
    {% endwith %}
  {% endif %}

  {# write symbol indices for global namespace #}
  {% if globalsIndex.all %}
    {% with page=globalsIndex scope='file' template='htmlflmembers.tpl' %}
      {% indexentry nav name=tr.fileMembers file=page.fileName anchor='' %}
      {% include 'htmlmembersindex.tpl' %}
    {% endwith %}
  {% endif %}

{% closesubindex nav %}

{# write directory documentation pages #}
{% for compound in dirList %}
  {% with page=compound %}
    {% create compound.fileName|append:config.HTML_FILE_EXTENSION from 'htmldir.tpl' %}
  {% endwith %}
{% endfor %}

{# close the global navigation index #}
{% closesubindex nav %}

{# write the navigation tree data #}
{% if config.GENERATE_TREEVIEW %}
  {% create 'navtreedata.js' from 'htmljsnavtree.tpl' %}
{% endif %}

{% msg %}----- End generating HTML output for from template ----{% endmsg %}
