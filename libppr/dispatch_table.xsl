<?xml version="1.0"?>
<stylesheet
	xmlns="http://www.w3.org/1999/XSL/Transform"
	version="1.0"
	xmlns:str="http://exslt.org/strings"
	xmlns:exsl="http://exslt.org/common"
	extension-element-prefixes="exsl"
	>
<output method="text"/>

<key name="toplevel" match="/dispatch/command/name/word[1]" use="."/>
<key name="helptopics" match="/dispatch/helptopic" use="name"/>

<template name="header_comments">
	<text>/* Auto-generated, do not edit.  Content is generated by&#10;</text>
	<text>   extracting XML command descriptions from comments in the&#10;</text>
	<text>   C source code and using xsltproc to format it with the&#10;</text>
	<text>   style-sheet dispatch_table.xsl. */&#10;</text>
</template>

<template match="/">
	<call-template name="header_comments"/>
	<text>#include &lt;config.h&gt;&#10;</text>
	<text>#include &lt;gu.h&gt;&#10;</text>
	<text>#include &lt;global_defines.h&gt;&#10;</text>
	<text>#include "dispatch.h"&#10;</text>
	<text>#include "dispatch_table.h"&#10;</text>

	<!-- Generate include file describing the command implementation functions. -->
	<exsl:document href="dispatch_table.h" method="text">
		<call-template name="header_comments"/>
		<for-each select="/dispatch/command">
			<text>int command</text><apply-templates select="name"/><text>(const char *argv[]);&#10;</text>
		</for-each>
	</exsl:document>

	<!-- Generate the command argument lists. -->
	<apply-templates select="/dispatch/command/args"/>

	<!-- The for sub-sub commands such as "group show".  This requires 
	     generating a unique list of the first words of two-word 
	     commands. -->
	<for-each select="/dispatch/command/name/word[generate-id(.)=generate-id(key('toplevel',.)[1])]">
		<if test="following-sibling::*">	<!-- if there is a second word to this command, -->
			<text>static struct COMMAND_NODE command_</text><value-of select="."/><text>[] = {&#10;</text>
			<for-each select="key('toplevel',.)">
				<apply-templates select="../.."/>
			</for-each>
			<text> {NULL}&#10;</text>
			<text> };&#10;</text>
		</if>
	</for-each>

	<!-- Generate the top level dispatch table. -->
	<text>struct COMMAND_NODE commands[] = {&#10;</text>
	<for-each select="/dispatch/command/name/word[generate-id(.)=generate-id(key('toplevel',.)[1])]">
		<choose>
			<!-- If this is a multi-word command, generate a branch node. -->
			<when test="following-sibling::*">
				<text> {"</text><value-of select="."/><text>", </text>
				<text>COMMAND_NODE_BRANCH, </text>
				<text>&amp;command_</text><value-of select="."/>
				<text>},&#10;</text>
			</when>
			<!-- If single word command, invoke /dispatch/command template 
			     to generate a leaf node. -->
			<otherwise>
				<apply-templates select="../.."/>
			</otherwise>
		</choose>
	</for-each>
	<text> {NULL}&#10;</text>
	<text> };&#10;</text>

	<!-- Generate list of help topics. -->
	<text>struct COMMAND_HELP commands_help_topics[] = {&#10;</text>
		<apply-templates select="/dispatch/helptopic"/>
	<text> {NULL}&#10;</text>
	<text> };&#10;</text>

</template>

<!-- Print a command name as a partial variable name. -->
<template match="/dispatch/command/name">
	<for-each select="word">
		<text>_</text><value-of select="."/>
	</for-each>
</template>

<!-- Emmit the initializer for an instance of struct COMMAND_NODE 
     which describes this command. -->
<template match="/dispatch/command">
	<text> {"</text><value-of select="name/word[position()=last()]"/><text>", </text>
	<text>COMMAND_NODE_LEAF, </text>
	<text>&amp;command</text><apply-templates select="name"/><text>_args, </text>
	<text>N_("</text><value-of select="desc"/><text>"), </text>
	<text>command</text><apply-templates select="name"/>

	<text>, </text>

	<!-- Generate helptopics bitmap.  We make the C compiler do the low-
	     level bit operations.  We start with a zero so we can lead
	     off each bit with a bitwise or operator. -->
	<text>0</text>
	<if test="@helptopics">
		<call-template name="split_helptopics">
			<with-param name="string" select="@helptopics"/>
		</call-template>
	</if>

	<text>, </text>

	<!-- If an ACL controls access to this command, emmit its name
	     as a string.  Otherwise, emmit NULL. -->
	<choose>
		<when test="@acl">
			<text>"</text><value-of select="@acl"/><text>"</text>
		</when>
		<otherwise>
			<text>NULL</text>
		</otherwise>
	</choose>

	<text>},&#10;</text>
</template>

<!-- We have to go to all this trouble because str:split() is so deaply
     flawed that it doesn't work in any but the most simple cases. -->
<template name="split_helptopics">
	<param name="string"/>
	<choose>
		<when test="contains($string,',')">
			<call-template name="split_helptopics">
				<with-param name="string" select="substring-before($string,',')"/>
			</call-template>
			<call-template name="split_helptopics">
				<with-param name="string" select="substring-after($string,',')"/>
			</call-template>
		</when>
		<otherwise>
			<text>|(1&lt;&lt;</text>
			<choose>
				<when test="key('helptopics',$string)">
					<value-of select="count(key('helptopics',$string)[1]/preceding-sibling::helptopic)"/>
				</when>
				<otherwise>	<!-- provoke error during compile -->
					<text>undefined</text>
				</otherwise>
			</choose>
			<text>)/*</text><value-of select="$string"/><text>*/</text>
		</otherwise>
	</choose>	
</template>

<!-- Create an array of struct COMMAND_ARG which describes the arguments for this command -->
<template match="/dispatch/command/args">
	<text>static struct COMMAND_ARG command</text><apply-templates select="../name"/><text>_args[] = {&#10;</text>
		<for-each select="arg">
			<text> {"</text><value-of select="name"/><text>", </text>
			<!-- Convert a set of flag words into flag bits.  These values
			     coorespond to the values in dispatch(). -->
			<text>0</text>
				<for-each select="str:split(@flags,',')">
					<choose>
						<when test=".='optional'"><text>|1</text></when>
						<when test=".='repeat'"><text>|2</text></when>
						<otherwise><value-of select="."/></otherwise>
					</choose>
				</for-each>
				<text>, </text>
			<text>N_("</text><value-of select="desc"/><text>")},</text>
			<text>&#10;</text>
		</for-each>
	<text> {NULL}&#10;</text>
	<text> };&#10;</text>
</template>

<template match="/dispatch/helptopic">
	<text> {</text>
	<text>"</text><value-of select="name"/><text>", </text>
	<text>N_("</text><apply-templates select="desc"/><text>")</text>
	<text>},&#10;</text>
</template>

</stylesheet>
