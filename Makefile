.PHONY: build clean

clean:
	rm -rf dist pachi_py.egg-info

upload: clean
	rm -rf dist
	python setup.py sdist
	twine upload dist/*
