#! /usr/bin/perl
#
# Dot matrix printer test.
#

# reset printer
printf("%c@",27);

# Print italic and bold samples
printf("This is %c4italic%c5 text.\r\n",27,27);

# Print colour samples
for($m=0; $m < 8; $m++)
	{
	printf("%cr%cTHIS IS COLOUR %d\r\n",27,$m,$m);
	}
printf("%cr%c",27,0);		# return to black
print "\r\n";

# Part expanded
printf("%cW%cEXPANDED %cW%cNORMAL\r\n",27,1,27,0);

# One line expanded
printf("%cOne line expanded\r\n",14);

# extra sizes
print "This is standard 10 cpi print\r\n";
printf("%cE%c",28,1);
print "THIS IS DOUBLE HORIZONTAL\r\n";
printf("%cE%c",28,2);
print "TRIPLE HORIZONTAL\r\n";
printf("%cE%c",28,1);
printf("%cV%c",28,1);
print "THIS IS 2X VERT & 2X HORZ\r\n";
printf("%cE%c",28,2);
print "2X VERT & 3X HORZ\r\n";
printf("%cE%c",28,0);
printf("%cV%c",28,0);
print "Back to standard print\r\n";
print "\r\n";

# Inter-character spacing
for($n=0; $n <= 10; $n++)
	{
	printf("%c %cAt this spacing (n) equals %d\r\n",27,$n,$n);
	}
printf("%c@",27);

# Line spacing
for($i=2; $i <=60; $i+=2)
	{
	printf("%c3%c%d/360 - inch line spacing\r\n",28,$i,$i);
	}

printf("%c@",27);

# Grapics modes
printf("%c",12);

sub g8pin
	{
	print "This is 8-pin grapics, M = $m ";
	printf("%c*%c%c%c",27,$m,60,0);
	for($z=1; $z <= 60; $z++)
		{
		printf("%c",255);
		}
	print "\r\n\r\n";
	}

sub g24pin
	{
	print "This is 24-pin grapics, M = $m ";
	printf("%c*%c%c%c",27,$m,60,0);
	for($z=1; $z <= 180; $z++)
		{
		printf("%c",255);
		}
	print "\r\n\r\n";
	}

# Test 8 pin graphics modes
for($m=0; $m <= 4; $m++)
	{ &g8pin(); }
$m=6; &g8pin();
print "\r\n";

# Test 24 pin graphics modes
$m=32; &g24pin();
$m=33; &g24pin();
$m=38; &g24pin();
$m=39; &g24pin();
$m=40; &g24pin();

exit(0);
