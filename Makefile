
Pipfile.lock:
	- pipenv --rm
	pipenv install

# should pass value of -j to scons
test: Pipfile.lock
	pipenv run scons run_utest::

all: Pipfile.lock
	pipenv run scons all