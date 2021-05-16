
import xmllib
import yagahost

class ElementAttribute(object):
	def __init__(self, name, value):
		self.name = name
		self.value = value

class ParserImpl(xmllib.XMLParser):
	def __init__(self):
		xmllib.XMLParser.__init__(self)
		self.errorString = ''
		self.contentHandler = None
	def ParseFile(self, path):
		assert self.contentHandler
		self.errorString = ''
		self.reset()
		f = yagahost.OpenAsset(path)
		if not f:
			return False
		try:
			data = f.read()
			data = data.replace('xmlns="x-schema:room_layout.xsd"', '')
			data = data.replace('xmlns="x-schema:menu_layout.xsd"', '')
			self.feed(data)
			self.close()
		except Exception, e:
			self.errorString = e.getMessage()
			return False
		return True
	def unknown_starttag(self, tag, attributes):
		l = []
		for key in attributes.keys():
			l.append(ElementAttribute(key, attributes[key]))
		self.contentHandler.StartElement(tag, l)
	def unknown_endtag(self, tag):
		self.contentHandler.EndElement(tag)
	def handle_data(self, data):
		self.contentHandler.Text(data)
