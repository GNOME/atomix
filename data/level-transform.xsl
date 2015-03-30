<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output indent="yes"/>
<xsl:strip-space elements="*"/>
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

  <xsl:template match="environment">
    <environment n_columns="{n_columns/text()}" n_rows="{n_rows/text()}">
      <xsl:apply-templates />
    </environment>
  </xsl:template>
  <xsl:template match="scenario">
    <scenario n_columns="{n_columns/text()}" n_rows="{n_rows/text()}">
      <xsl:apply-templates />
    </scenario>
  </xsl:template>
  <xsl:template match="goal">
    <goal n_columns="{n_columns/text()}" n_rows="{n_rows/text()}">
      <xsl:apply-templates />
    </goal>
  </xsl:template>
  <xsl:template match="row">
     <xsl:apply-templates />
  </xsl:template>
  <xsl:template match="col">
    <position>
      <xsl:attribute name="row">
        <xsl:value-of select="../@no" />
      </xsl:attribute>
      <xsl:attribute name="col">
        <xsl:value-of select="@no" />
      </xsl:attribute>
      <xsl:apply-templates />
    </position>
  </xsl:template>
  <xsl:template match="tile">
     <tile type="{type/text()}" base="{base/text()}">
       <xsl:apply-templates />
     </tile>
  </xsl:template>
  <xsl:template match="n_columns|n_rows|type|base"/>

</xsl:stylesheet>
