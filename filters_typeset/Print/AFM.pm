#
# AFM.pm
#
# Last modified 15 January 1999.
#

#
# This module contains a class for reading an AFM (Adobe Font Metrics) file.
#

package Print::AFM;

# These are the names of some member functions which can be used to
# get information about the font.  They directly coorespond to
# keys in the AFM file.
%IMPORTANT = qw(
FontName		str
FullName		str
FamilyName		str
Weight			str
ItalicAngle		int
IsFixedPitch		bool
FontBBox		intarray
UnderlinePosition	int
UnderlineThickness	int
Version 		str
EncodingScheme 		str
CapHeight 		int
XHeight 		int
Ascender 		int
Descender 		int
);

#
# Create a new object.  If a filename argument is provided,
# load the data from the file into the object.
#
sub new
    {
    my $obj = {};
    bless $obj;

    my $filename = shift;
    if(defined($filename))
    	{ $obj->load($filename) }

    return $obj;
    }

#
# Load the data from an AFM file into the object.
#
sub load
    {
    my $self = shift;
    my $filename = shift;

    $self->{filename} = $filename;

    open(AFM, "< $filename") || die "File \"$filename\" can't be opened, $!";

    my $temp;
    while(<AFM>)
	{
	if(/^C /)
	    {
	    my($name, $code, $width, $bbox);
	    foreach $temp (split(/\s*;\s*/))
	    	{
		if($temp =~ /^C\s+([-0-9]+)/)
		    { $code = $1 }
		elsif($temp =~ /^N\s+(\S+)/)
		    { $name = $1 }
		elsif($temp =~ /^WX\s+([-0-9]+)/)
		    { $width = $1 }
		elsif($temp =~ /^B\s+([-0-9]+)\s+([-0-9]+)\s+([-0-9]+)\s+([-0-9]+)/)
		    { $bbox = [$1, $2, $3, $4] }
		elsif($temp =~ /^L\s+/)
		    { }
		else
		    { die "Unrecognized AFM character fact \"$temp\" in file \"$self->{filename}\"" }
	    	}
	    $self->{"_$name"} = Print::AFM::Char::new($code, $name, $width, $bbox);
	    }
	elsif(/^CC /)
	    {
	    }
	elsif(/^(\S+)\s+(.*)$/)
	    {
	    if(defined($temp = $IMPORTANT{$1}))
	    	{
		if($temp eq 'int' || $temp eq 'str')
		    {
		    $self->{$1} = $2;
		    }
		elsif($temp eq 'bool')
		    {
		    my($name, $value) = ($1, $2);
		    $self->{$name} = ($value =~ /^[tT]/) ? 1 : 0;
		    }
		elsif($temp eq 'intarray')
		    {
		    $self->{$1} = \split(/\s+/, $2);
		    }
		else
		    {
		    die;
		    }
	    	}
	    }
	}

    close(AFM) || die;
    }

#
# This autoload subroutine is called automatically for undefined
# member functions.  It does the work of the missing function.
sub AUTOLOAD
    {
    my @t = split(/::/, $AUTOLOAD);
    my $funct = pop(@t);
    if(defined($IMPORTANT{$funct}))
    	{
	my $self = shift;
	my $result = $self->{$funct};
	if(!defined($result))
	    { die("Item \"$funct\" missing from AFM file \"$self->{filename}\"") }
	return $result;
    	}
    else
    	{
    	die "No member function called \"$funct\"";
    	}
    }

#
# Retrieve one of the character objects
#
sub getchar
    {
    my $self = shift;
    my $charname = shift;
    return $self->{"_$charname"};
    }

#
# Definition of a character object
#
package Print::AFM::Char;

sub new
    {
    my($code, $name, $width, $bbox) = @_;

    die if(!defined($code));
    die if(!defined($name));
    die if(!defined($width));
    die if(!defined($bbox));

    my $self = {
    	'code' => $code,
    	'name' => $name,
    	'width' => $width,
    	'bbox' => $bbox };

    bless $self;
    return $self;
    }

sub code { return shift->{code} }
sub name { return shift->{name} }
sub width { return shift->{width} }
sub bbox { return shift->{bbox} }

1;


