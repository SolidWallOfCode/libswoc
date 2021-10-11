AUTOMAKE_OPTIONS = nostdinc
AM_CXXFLAGS = -std=c++17 -I $(top_srcdir)/code/include
EXTRA_PROGRAMS = ex_lru_cache

ex_lru_cache_SOURCES = \
	ex_lru_cache.cc 

ex_lru_cache_LDADD = $(top_builddir)/code/src/libswoc-static-1.3.1.a -lpthread


