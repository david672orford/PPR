#
# mouse:~ppr/src/filters_typeset/Print/SP.pl
# Copyright 1995--1999 Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 28 July 1999.
#

#
# The scaled point is TeX's basic unit of measure.
# There are 65536 scaled points to a printers point
# which TeX considers to be 1/72.27th of an inch.
#
# This unit accomodates several levels of infinity so
# that it can be used to represent the size of TeX "glue".
# The level of infinity for ordinary dimensions is 0.
#

#int value;	# size in TeX internal units
#int infinity;	# level of glue infinity
package Print::SP;

# allows direct assignment
sub new
    {
    my $value = shift;
    my $self = {};

    if(ref($value) eq "SCALAR")
    	{
    	my $infinity = shift;
	$self->{value} = $value;
	if(defined($infinity))
	    { $self->{infinity} = $infinity }
	else
	    { $self->{infinity} = 0 }
	}
    else
    	{
        $self->{value} = $value->GetValue();
        $self->{infinity} = $values->GetInfinity();
        }
    return bless $self;
    }

# allows us to retrieve the value
sub GetValue
    {
    my $self = shift;
    return $self->{value};
    }

# Infinity level manipulation
sub SetInfinity
    {
    my $self = shift;
    my $iinf = shift;
    $self->{infinity} = $iinf;
    }

sub GetInfinity
    {
    my $self = shift;
    return $self->{infinity};
    }

#
# Functions to convert other dimensions expressed
# as doubles or ints to scaled points:
#
sub PT { return new Print::SP(int((shift) *   65536.0)) }
sub PC { return new Print::SP(int((shift) *  473629.0)) }
sub IN { return new Print::SP(int((shift) * 4736287.0)) }
sub BP { return new Print::SP(int((shift) *   65782.0)) }
sub CM { return new Print::SP(int((shift) * 1864679.0)) }
sub MM { return new Print::SP(int((shift) *  186468.0)) }
sub DD { return new Print::SP(int((shift) *   61248.0)) }
sub CC { return new Print::SP(int((shift) *  734977.0)) }

# addition
sub add
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    # If we are "more infinite", then we prevail:
    if($self->{infinity} > $Arg->{infinity})
	{ return $self }

    # If it is "more infinite", then it prevails:
    elsif($Arg->{infinity} > $self->{infinity})
	{ return $Arg }

    # If the infinity level is the same then we
    # will do real addition.
    else
	{
	my $temp = {value => ($self->{value} + $Arg->{value}),
		infinity => $self->{infinity}};
	return bless $temp;
	}
    }

# add and set
sub incr
    {
    my $self = shift;
    my $Arg = shift;

    if($self->{infinity} < $Arg->{infinity})
	{
	$self->{infinity} = $Arg->{infinity};
	$self->{value} = $Arg->{value};
	}
    elsif($self->{infinity} == $Arg->{infinity})
	{
	$self->{value} += $Arg->{value};
	}

    return $self;
    }

# subtraction
sub subt
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    if($self->{infinity} > $Arg->{infinity})	# subtracting from a greater infinity
	{
	return $self;				# accomplishes nothing
	}

    elsif($self->{infinity} < $Arg->{infinity})	# subtracting a greater infinity
	{						# yields the oposite of the greater
	my $temp = { value => (0 - $Arg->{value}),	# infinity
		infinity => $Arg->{infinity} };
	return bless $temp;
	}

    else					# equal infinities
	{					# need simple subtraction
	my $temp = { value => ($self->{value} - $Arg->{value}),
		infinity => $self->{infinity} };
	return bless $temp;
	}
    }

# subtract and set
sub decr
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    if($self->{infinity} < $Arg->{infinity})		# subtracting infinity
	{
	$self->{infinity} = $Arg->{infinity};		# from < inf
	$self->{value} = (0 - $Arg->{value});    	# yields negative infinity
	}
    elsif($self->{infinity} == $Arg->{infinity})
	{
	$self->{value} -= $Arg->{value};
	}
    return $self;
    }

# Multiply
sub mul
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    my $temp = { infinity => ($self->{infinity} + $Arg->{infinity}),
        value => ($self->{value} * $Arg->{value}) };

    return bless $temp;
    }

# Divide
sub div
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    my $temp = { value => ($self->{value} / $Arg->{value}),
		infinity => ($self->{infinity} - $Arg->{infinity}) };

    return bless $temp;
    }

# Greater than
sub gt
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    if($self->{infinity} > $Arg->{infinity})	# if infinity order of left op
	{ return 1 }				# is greater, it is greater

    return $self->{value} > $Arg->{value};
    }

# Less than
sub lt
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    if($self->{infinity} < $Arg->{infinity})	# if left op is less infinite
	{ return 1 }				# it is less

    return $self->{value} < $Arg->{value};
    }

# Equal
sub eq
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    if($self->{infinity} != $Arg->{infinity})	# if infinities differ
	{ return 0 }				# they are not equal

    return $self->{value} == $Arg->{value};
    }

# Not equal
sub ne
    {
    my $self = shift;
    my $Arg = shift;

    if(ref($Arg) eq 'SCALAR')
	{ $Arg = new Print::SP($Arg) }

    if($self->{infinity} != $Arg->{infinity})	# if infinities differ,
	{ return 1 }				# they are not equal

    return $self->{value} != $Arg->{value};
    }

#
# Output routine for SP class.
# This displays an SP value in human readable form.
#
sub tostr
    {
    my $self = shift;
    my $handle = shift;

    # If the number is infinite, print the level
    # of infinity.
    if($self->{infinity})
	{ print $handle "INF $self->{infinity} " }

    # Print in scaled points.
    print $handle "$self->{value}sp";

    # Print in printer's points.
    #return str << setprecision(4) << ((double)($Arg->GetValue()) / 65536.0);
    }

# end of file

