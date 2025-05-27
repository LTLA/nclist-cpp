<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.12.0">
  <compound kind="file">
    <name>build.hpp</name>
    <path>nclist/</path>
    <filename>build_8hpp.html</filename>
    <class kind="struct">nclist::Nclist</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>nclist.hpp</name>
    <path>nclist/</path>
    <filename>nclist_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <includes id="overlaps__any_8hpp" name="overlaps_any.hpp" local="yes" import="no" module="no" objc="no">overlaps_any.hpp</includes>
    <includes id="overlaps__end_8hpp" name="overlaps_end.hpp" local="yes" import="no" module="no" objc="no">overlaps_end.hpp</includes>
    <includes id="overlaps__equal_8hpp" name="overlaps_equal.hpp" local="yes" import="no" module="no" objc="no">overlaps_equal.hpp</includes>
    <includes id="overlaps__extend_8hpp" name="overlaps_extend.hpp" local="yes" import="no" module="no" objc="no">overlaps_extend.hpp</includes>
    <includes id="overlaps__start_8hpp" name="overlaps_start.hpp" local="yes" import="no" module="no" objc="no">overlaps_start.hpp</includes>
    <includes id="overlaps__within_8hpp" name="overlaps_within.hpp" local="yes" import="no" module="no" objc="no">overlaps_within.hpp</includes>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>overlaps_any.hpp</name>
    <path>nclist/</path>
    <filename>overlaps__any_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <class kind="struct">nclist::OverlapsAnyWorkspace</class>
    <class kind="struct">nclist::OverlapsAnyParameters</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>overlaps_end.hpp</name>
    <path>nclist/</path>
    <filename>overlaps__end_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <class kind="struct">nclist::OverlapsEndWorkspace</class>
    <class kind="struct">nclist::OverlapsEndParameters</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>overlaps_equal.hpp</name>
    <path>nclist/</path>
    <filename>overlaps__equal_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <class kind="struct">nclist::OverlapsEqualWorkspace</class>
    <class kind="struct">nclist::OverlapsEqualParameters</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>overlaps_extend.hpp</name>
    <path>nclist/</path>
    <filename>overlaps__extend_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <class kind="struct">nclist::OverlapsExtendWorkspace</class>
    <class kind="struct">nclist::OverlapsExtendParameters</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>overlaps_start.hpp</name>
    <path>nclist/</path>
    <filename>overlaps__start_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <class kind="struct">nclist::OverlapsStartWorkspace</class>
    <class kind="struct">nclist::OverlapsStartParameters</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="file">
    <name>overlaps_within.hpp</name>
    <path>nclist/</path>
    <filename>overlaps__within_8hpp.html</filename>
    <includes id="build_8hpp" name="build.hpp" local="yes" import="no" module="no" objc="no">build.hpp</includes>
    <class kind="struct">nclist::OverlapsWithinWorkspace</class>
    <class kind="struct">nclist::OverlapsWithinParameters</class>
    <namespace>nclist</namespace>
  </compound>
  <compound kind="struct">
    <name>nclist::Nclist</name>
    <filename>structnclist_1_1Nclist.html</filename>
    <templarg>typename Index_</templarg>
    <templarg>typename Position_</templarg>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsAnyParameters</name>
    <filename>structnclist_1_1OverlapsAnyParameters.html</filename>
    <templarg>typename Position_</templarg>
    <member kind="variable">
      <type>std::optional&lt; Position_ &gt;</type>
      <name>max_gap</name>
      <anchorfile>structnclist_1_1OverlapsAnyParameters.html</anchorfile>
      <anchor>a3cbcc727a8b0a7901edf133658e083a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>Position_</type>
      <name>min_overlap</name>
      <anchorfile>structnclist_1_1OverlapsAnyParameters.html</anchorfile>
      <anchor>abebf28537e480888e34414056b434958</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>quit_on_first</name>
      <anchorfile>structnclist_1_1OverlapsAnyParameters.html</anchorfile>
      <anchor>a8f06f6eff0a683f5cb0e32060bd5f5f3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsAnyWorkspace</name>
    <filename>structnclist_1_1OverlapsAnyWorkspace.html</filename>
    <templarg>typename Index_</templarg>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsEndParameters</name>
    <filename>structnclist_1_1OverlapsEndParameters.html</filename>
    <templarg>typename Position_</templarg>
    <member kind="variable">
      <type>Position_</type>
      <name>max_gap</name>
      <anchorfile>structnclist_1_1OverlapsEndParameters.html</anchorfile>
      <anchor>a9044d1c7f2b874afa173d509bb7ff4a9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>Position_</type>
      <name>min_overlap</name>
      <anchorfile>structnclist_1_1OverlapsEndParameters.html</anchorfile>
      <anchor>a16c0568bc0bc34cceb0c1a8ddcc68be3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>quit_on_first</name>
      <anchorfile>structnclist_1_1OverlapsEndParameters.html</anchorfile>
      <anchor>a08a8d4f230b4786dd7886be20b634e5c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsEndWorkspace</name>
    <filename>structnclist_1_1OverlapsEndWorkspace.html</filename>
    <templarg>typename Index_</templarg>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsEqualParameters</name>
    <filename>structnclist_1_1OverlapsEqualParameters.html</filename>
    <templarg>typename Position_</templarg>
    <member kind="variable">
      <type>Position_</type>
      <name>max_gap</name>
      <anchorfile>structnclist_1_1OverlapsEqualParameters.html</anchorfile>
      <anchor>a0b5cababa232c36917e4df6638a00547</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>Position_</type>
      <name>min_overlap</name>
      <anchorfile>structnclist_1_1OverlapsEqualParameters.html</anchorfile>
      <anchor>a5ec1ee3a73d0590e39b593425f4a382a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>quit_on_first</name>
      <anchorfile>structnclist_1_1OverlapsEqualParameters.html</anchorfile>
      <anchor>ac5ca81d6a81bfe6df3d3853805d52ff8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsEqualWorkspace</name>
    <filename>structnclist_1_1OverlapsEqualWorkspace.html</filename>
    <templarg>typename Index_</templarg>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsExtendParameters</name>
    <filename>structnclist_1_1OverlapsExtendParameters.html</filename>
    <templarg>typename Position_</templarg>
    <member kind="variable">
      <type>std::optional&lt; Position_ &gt;</type>
      <name>max_gap</name>
      <anchorfile>structnclist_1_1OverlapsExtendParameters.html</anchorfile>
      <anchor>a81bca092d4fc66aaaeada7b13cec78f6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>Position_</type>
      <name>min_overlap</name>
      <anchorfile>structnclist_1_1OverlapsExtendParameters.html</anchorfile>
      <anchor>afbb82777e1b948e6b037713c88de94c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>quit_on_first</name>
      <anchorfile>structnclist_1_1OverlapsExtendParameters.html</anchorfile>
      <anchor>a6bce1fdfe4fefab5c48a194d55d02994</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsExtendWorkspace</name>
    <filename>structnclist_1_1OverlapsExtendWorkspace.html</filename>
    <templarg>typename Index_</templarg>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsStartParameters</name>
    <filename>structnclist_1_1OverlapsStartParameters.html</filename>
    <templarg>typename Position_</templarg>
    <member kind="variable">
      <type>Position_</type>
      <name>max_gap</name>
      <anchorfile>structnclist_1_1OverlapsStartParameters.html</anchorfile>
      <anchor>aaf56a28fba57e919f94088408bc03eb4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>Position_</type>
      <name>min_overlap</name>
      <anchorfile>structnclist_1_1OverlapsStartParameters.html</anchorfile>
      <anchor>a0522dd6f35f283fc8638a9d20a88da8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>quit_on_first</name>
      <anchorfile>structnclist_1_1OverlapsStartParameters.html</anchorfile>
      <anchor>a26b33e5a64909962350b7369cd431dcf</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsStartWorkspace</name>
    <filename>structnclist_1_1OverlapsStartWorkspace.html</filename>
    <templarg>typename Index_</templarg>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsWithinParameters</name>
    <filename>structnclist_1_1OverlapsWithinParameters.html</filename>
    <templarg>typename Position_</templarg>
    <member kind="variable">
      <type>std::optional&lt; Position_ &gt;</type>
      <name>max_gap</name>
      <anchorfile>structnclist_1_1OverlapsWithinParameters.html</anchorfile>
      <anchor>ac1ea64811fa56a9692d7fa76c86e6f7c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>Position_</type>
      <name>min_overlap</name>
      <anchorfile>structnclist_1_1OverlapsWithinParameters.html</anchorfile>
      <anchor>ad52f540fa13a914bd4a53e6029a74ad6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>quit_on_first</name>
      <anchorfile>structnclist_1_1OverlapsWithinParameters.html</anchorfile>
      <anchor>a2b0d57f83eea2872bf1fc766b4c86bbe</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>nclist::OverlapsWithinWorkspace</name>
    <filename>structnclist_1_1OverlapsWithinWorkspace.html</filename>
    <templarg>typename Index_</templarg>
  </compound>
  <compound kind="namespace">
    <name>nclist</name>
    <filename>namespacenclist.html</filename>
    <class kind="struct">nclist::Nclist</class>
    <class kind="struct">nclist::OverlapsAnyParameters</class>
    <class kind="struct">nclist::OverlapsAnyWorkspace</class>
    <class kind="struct">nclist::OverlapsEndParameters</class>
    <class kind="struct">nclist::OverlapsEndWorkspace</class>
    <class kind="struct">nclist::OverlapsEqualParameters</class>
    <class kind="struct">nclist::OverlapsEqualWorkspace</class>
    <class kind="struct">nclist::OverlapsExtendParameters</class>
    <class kind="struct">nclist::OverlapsExtendWorkspace</class>
    <class kind="struct">nclist::OverlapsStartParameters</class>
    <class kind="struct">nclist::OverlapsStartWorkspace</class>
    <class kind="struct">nclist::OverlapsWithinParameters</class>
    <class kind="struct">nclist::OverlapsWithinWorkspace</class>
    <member kind="function">
      <type>Nclist&lt; Index_, Position_ &gt;</type>
      <name>build</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>af017ae50c229346e3cf8e41599fbb5c0</anchor>
      <arglist>(Index_ num_subset, const Index_ *subset, const Position_ *starts, const Position_ *ends)</arglist>
    </member>
    <member kind="function">
      <type>Nclist&lt; Index_, Position_ &gt;</type>
      <name>build</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>aa77737736d32ca4e0fec076d92c64665</anchor>
      <arglist>(Index_ num_intervals, const Position_ *starts, const Position_ *ends)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>overlaps_any</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>ade7d2e27bd85d84a0cd210d3fcce7335</anchor>
      <arglist>(const Nclist&lt; Index_, Position_ &gt; &amp;subject, Position_ query_start, Position_ query_end, const OverlapsAnyParameters&lt; Position_ &gt; &amp;params, OverlapsAnyWorkspace&lt; Index_ &gt; &amp;workspace, std::vector&lt; Index_ &gt; &amp;matches)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>overlaps_end</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>a26c1a70487c3d4a5243622b1bf757523</anchor>
      <arglist>(const Nclist&lt; Index_, Position_ &gt; &amp;subject, Position_ query_start, Position_ query_end, const OverlapsEndParameters&lt; Position_ &gt; &amp;params, OverlapsEndWorkspace&lt; Index_ &gt; &amp;workspace, std::vector&lt; Index_ &gt; &amp;matches)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>overlaps_equal</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>a749d236b974c3bacc843cc96548d7e20</anchor>
      <arglist>(const Nclist&lt; Index_, Position_ &gt; &amp;subject, Position_ query_start, Position_ query_end, const OverlapsEqualParameters&lt; Position_ &gt; &amp;params, OverlapsEqualWorkspace&lt; Index_ &gt; &amp;workspace, std::vector&lt; Index_ &gt; &amp;matches)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>overlaps_extend</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>a6c93cd0ba40906272d62411f3a7b83c3</anchor>
      <arglist>(const Nclist&lt; Index_, Position_ &gt; &amp;subject, Position_ query_start, Position_ query_end, const OverlapsExtendParameters&lt; Position_ &gt; &amp;params, OverlapsExtendWorkspace&lt; Index_ &gt; &amp;workspace, std::vector&lt; Index_ &gt; &amp;matches)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>overlaps_start</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>aebdbb3bf7c28aad8d1237d030a6c97f6</anchor>
      <arglist>(const Nclist&lt; Index_, Position_ &gt; &amp;subject, Position_ query_start, Position_ query_end, const OverlapsStartParameters&lt; Position_ &gt; &amp;params, OverlapsStartWorkspace&lt; Index_ &gt; &amp;workspace, std::vector&lt; Index_ &gt; &amp;matches)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>overlaps_within</name>
      <anchorfile>namespacenclist.html</anchorfile>
      <anchor>a17c768a993d651cfa8bd50080cfdd913</anchor>
      <arglist>(const Nclist&lt; Index_, Position_ &gt; &amp;subject, Position_ query_start, Position_ query_end, const OverlapsWithinParameters&lt; Position_ &gt; &amp;params, OverlapsWithinWorkspace&lt; Index_ &gt; &amp;workspace, std::vector&lt; Index_ &gt; &amp;matches)</arglist>
    </member>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title>Nested containment lists in C++</title>
    <filename>index.html</filename>
    <docanchor file="index.html" title="Nested containment lists in C++">md__2github_2workspace_2README</docanchor>
  </compound>
</tagfile>
