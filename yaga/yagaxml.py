
import sys

class IContentHandler(object):
	def __init__(self):
		pass
	def StartElement(self, name, attrs):
		pass
	def EndElement(self, name):
		pass
	def Text(self, text):
		pass
	def Comment(self, text):
		pass

def Parser():
	if sys.version_info[0] == 2 and sys.version_info[1] <= 2:
		import yagaxmllib
		return yagaxmllib.ParserImpl()
	else:
		import yagaxmlsax
		return yagaxmlsax.ParserImpl()

class XmlSystemImpl(object):
	def __init__(self):
		pass
	def CreateParser(self):
		return Parser()

g_xmlSystem = XmlSystemImpl()

def XmlSystem():
	return g_xmlSystem
