
import yagagraphics
import yagahost

class HorizontalJustification:
	HJUSTIFY_CENTER = 1
	HJUSTIFY_LEFT = 2
	HJUSTIFY_RIGHT = 3

class VerticalJustification:
	VJUSTIFY_CENTER = 1
	VJUSTIFY_TOP = 2
	VJUSTIFY_BOTTOM = 3

class ImageStringFlags:
	IMGSTR_CLIP = 1

class ImageString(object):
	def __init__(self):
	        self.font = None
		self.opacity = 1.
		self.position = None
		self.hJustify = None
		self.vJustify = None
		self.text = ''
	def SetText(self, text):
		# print('STUB: ImageString.SetText text:' + text)
		self.text = text
	def Seek(self, pos):
		# print('STUB: ImageString.Seek pos:' + str(pos))
		pass
	def Render(self, camera):
		# print('STUB: ImageString.Render')
		dstRect = yagagraphics.Rect(self.position.x, self.position.y)
		for c in self.text:
			yagahost.DrawFontChar(self.font._font, ord(c), dstRect.x, dstRect.y)
			r = yagahost.GetFontCharRect(self.font._font, ord(c))
			dstRect.x += r['w']

class AsciiData(object):
	def __init__(self):
		self.minChar = None
		self.maxChar = None
		self.spaceChar = None
		self.baselineChar = None
	def TotalChars(self):
		return self.maxChar - self.minChar + 1

class Font(object):
	def __init__(self, name, img, asciiData):
		assert img._anim
		self.id = name
		self._img = img
		self._asciiData = asciiData
		self._font = yagahost.LoadFont(img._anim.num, asciiData.minChar, asciiData.maxChar, asciiData.spaceChar)
	def GetStringRect(self, text):
		# print('STUB: Font.GetStringRect')
		w = 0
		h = 0
		for c in text:
			r = yagahost.GetFontCharRect(self._font, ord(c))
			w += r['w']
			h = max(h, r['h'])
		return yagagraphics.Rect(0, 0, w, h)

class FontManagerImpl(object):
	def __init__(self):
		self._fonts = {}
	def CreateImageString(self):
		# print('STUB: FontManagerImpl.CreateImageString')
		return ImageString()
	def GetFont(self, name):
		# print('STUB: FontManagerImpl.GetFont name:' + name)
		assert name in self._fonts
		return self._fonts[name]
	def CreateFontFromImage(self, img, name, asciiData):
		# print('STUB: FontManagerImpl.CreateFontFromImage')
		assert not (name in self._fonts)
		font = Font(name, img.img, asciiData)
		self._fonts[name] = font
		return font

g_fontManager = FontManagerImpl()

def FontManager():
	return g_fontManager
