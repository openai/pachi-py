.PHONY: build clean

clean:
	rm -rf dist pachi_py.egg-info build pachi_py/build

upload: clean
	rm -rf dist
	python setup.py sdist
	twine upload dist/*
