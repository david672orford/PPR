all:
	( cd .. && dpkg-buildpackage -rfakeroot )

clean:
	( cd .. && make -f debian/rules clean )
	rm -rf conffiles
	
