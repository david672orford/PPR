<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:template match="pod">
<![CDATA[
<!DOCTYPE refentry PUBLIC "-//OASIS//DTD DocBook XML V4.2//EN" "http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd">
]]>
  <RefEntry>
  <xsl:apply-templates/>
  </RefEntry>
</xsl:template>

<xsl:template match="head/title">
  <RefNameDiv>
  <xsl:apply-templates/>
  </RefNameDiv>
</xsl:template>

<xsl:template match="head/title/strong">
  <RefName>
  <xsl:apply-templates/>
  </RefName>
</xsl:template>

<xsl:template match="sect1">
  <RefSect1>
  <xsl:apply-templates/>
  </RefSect1>
</xsl:template>

<xsl:template match="title">
  <Title>
  <xsl:apply-templates/>
  </Title>
</xsl:template>

<xsl:template match="para">
  <Para>
  <xsl:apply-templates/>
  </Para>
</xsl:template>

<xsl:template match="filename">
  <Filename>
  <xsl:apply-templates/>
  </Filename>
</xsl:template>

<xsl:template match="strong">
  <Program>
  <xsl:apply-templates/>
  </Program>
</xsl:template>

</xsl:stylesheet>
