#! /usr/bin/perl

$line = "";
$eof = 0;
while(1)
	{
	$papname = "";
	$pprname = "";
	while(1)
		{
		if($line eq "")
			{
			exit 0 if($eof);
			if(defined($line = <STDIN>))
				{
				chomp $line;
				}
			else
				{
				$line = "";
				$eof = 1;
				last;
				}
			}
		if($line =~ /^\s*[;#]/)
			{
			$line = "";
			}
		elsif($line =~ /^\s*$/)
			{
			last;
			}
		elsif($line =~ /^\s*\[([^\]]+)\]\s*$/)
			{
			if($papname eq "")
				{
				print "# $line\n";
				$papname = $1;
				$papname =~ s/^"//;
				$papname =~ s/"$//;
				$line = "";
				}
			else
				{
				last;
				}
			}
		elsif($line =~ /^\s*pprname\s*=\s*(\S+)\s*$/i)
			{
			print "# $line\n";
			$pprname = $1;
			$line = "";
			}
		elsif($line =~ /^\s*papname\s*=\s*(.+?)\s*$/i)
			{
			print "# $line\n";
			$papname = $1;
			$papname =~ s/^"//;
			$papname =~ s/"$//;
			$line = "";
			}
		else
			{
			print "# $line (ignored)\n";
			$line = "";
			}
		}

	$pprname ne "" || die;

	if(-f "/etc/ppr/aliases/$pprname")
		{
		print "ppad alias addon papname $pprname \"$papname\"\n\n";
		}
	elsif(-f "/etc/ppr/groups/$pprname")
		{
		print "ppad group addon papname $pprname \"$papname\"\n\n";
		}
	elsif(-f "/etc/ppr/printers/$pprname")
		{
		print "ppad addon papname $pprname \"$papname\"\n\n";
		}
	else
		{
		die;
		}

	}


