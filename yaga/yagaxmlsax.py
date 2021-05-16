
import xml.sax
import yagahost

class ElementAttribute(object):
	def __init__(self, name, value):
		self.name = name
		self.value = value

class ContentHandler(xml.sax.handler.ContentHandler):
	def __init__(self, contentHandler):
		self.contentHandler = contentHandler
	def startElement(self, name, attrs):
		l = []
		for attrName in attrs.getNames():
			l.append(ElementAttribute(attrName, attrs.getValue(attrName)))
		self.contentHandler.StartElement(name, l)
	def endElement(self, name):
		self.contentHandler.EndElement(name)
	def characters(self, content):
		self.contentHandler.Text(content)

class ParserImpl(object):
	def __init__(self):
		self.errorString = ''
		self.contentHandler = None
	def ParseFile(self, path):
		assert self.contentHandler
		self.errorString = ''
		f = yagahost.OpenAsset(path)
		if not f:
			return False
		try:
			parse = xml.sax.parse(f, ContentHandler(self.contentHandler))
		except xml.sax.SAXException as e:
			self.errorString = e.getMessage()
			return False
		return True
