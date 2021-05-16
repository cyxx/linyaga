
import yagagraphics

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
		self.position = None
		self.hJustify = None
		self.vJustify = None
	def SetText(self, text):
		print('STUB: ImageString.SetText text:' + text)
	def Seek(self, pos):
		print('STUB: ImageString.Seek pos:' + str(pos))
	def Render(self, camera):
		print('STUB: ImageString.Render camera:' + str(camera))

class AsciiData(object):
	def __init__(self):
		pass

class Font(object):
	def __init__(self, name):
		self.id = name
	def GetStringRect(self, text):
		print('STUB: Font.GetStringRect')
		return yagagraphics.Rect()

class FontManagerImpl(object):
	def __init__(self):
		pass
	def CreateImageString(self):
		print('STUB: FontManagerImpl.CreateImageString')
		return ImageString()
	def GetFont(self, name):
		print('STUB: FontManagerImpl.GetFont name:' + name)
		return Font(name)
	def CreateFontFromImage(self, img, resId, asciiData):
		print('STUB: FontManagerImpl.CreateFontFromImage')
		return Font(resId)

g_fontManager = FontManagerImpl()

def FontManager():
	return g_fontManager
