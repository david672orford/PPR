#
# mouse:~ppr/src/www/cgi_back.pl
# Copyright 1995--2003, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 23 December 2003.
#

require 'cgi_redirect.pl';

#
# Return a new HIST encoded in QUERY_STRING form with the calling script as
# the last item.  This is the value which it should pass to its children.
#
sub cgi_back_init
	{
	if(!defined($data{HIST}))
		{
		if(defined($ENV{HTTP_REFERER}))
			{ $data{HIST} = $ENV{HTTP_REFERER} }
		else
			{ $data{HIST} = "/" }
		}

	return form_urlencoded("HIST", "$data{HIST} $ENV{SCRIPT_NAME}");
	}

#
# Attempt to load the last URL listed in HIST into the browser.
#
sub cgi_back_doit
	{
	if(!defined($data{HIST}))
		{
		require 'cgi_error.pl';
		error_doc("Can't Go Back", <<"EndOfText");;
<p>Attempt to return to previous screen failed because the URL of the previous
screen was lost.
</p>
EndOfText

		return;
		}

	my @back_stack_list = split(/ /, $data{HIST});
	my $back_url = pop(@back_stack_list);
	if($back_url =~ /^\//)
		{
		$back_url = "http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}$back_url";
		}
	#print STDERR "Going back to \"$back_url\"\n";

	cgi_redirect($back_url);
	}

1;

