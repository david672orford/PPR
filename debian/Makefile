all:
	( cd .. && dpkg-buildpackage -rfakeroot )

clean:
	#( cd .. && make -f debian/rules clean )
	debbuild clean
	rm -rf conffiles
	
