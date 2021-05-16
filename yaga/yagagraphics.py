
import yagahost

class GraphicsHardware:
	SYSTEM_SOFTWARE = 1
	SYSTEM_HARDWARE = 2

class PixelFormat:
	PXL_R5G6B5 = 1
	PXL_X1R5G5B5 = 2
	PXL_A1R5G5B5 = 3
	PXL_R8G8B8 = 4
	PXL_X8R8G8B8 = 5
	PXL_A8R8G8B8 = 6
	PXL_A8LUM8 = 7
	PXL_A8PAL8 = 8
	PXL_A8 = 9
	PXL_APAL8 = 10
	PXL_LUM8 = 11
	PXL_PAL8 = 12

class RenderHint:
	RENDER_DEFAULT = 1

class TargetType:
	TARGET_DEFAULT = 0
	TARGET_WINDOWED = 1
	TARGET_FULL_SCREEN = 2

class ClearFlags:
	CLEAR_BACK = 1

class Rect(object):
	def __init__(self, x = 0, y = 0, w = 0, h = 0):
		self.x = x
		self.y = y
		self.width = w
		self.height = h

class VideoMode(object):
	def __init__(self, width, height, fmt):
		self.width = width
		self.height = height
		self.format = fmt

class IRenderTarget(object):
	def __init__(self):
		pass

class RenderTarget(IRenderTarget):
	def __init__(self, videoMode, targetType):
		yagahost.SetScreenSize(videoMode.width, videoMode.height, videoMode.format)
		self.identity = 0
		self.mode = targetType
		self._title = ''
	def LoadCursorFile(self, path, resId):
		print('STUB: RenderTarget.LoadCursorFile path:' + path)
	def SetCursorByID(self, resId):
		print('STUB: RenderTarget.SetCursorByID resId:' + str(resId))
	def RenderBegin(self, clearFlags):
		yagahost.ClearScreen()
	def RenderEnd(self):
		yagahost.UpdateScreen()
	def RenderImage(self, imageBuffer, opacity, dstRect, srcRect):
		print('STUB: RenderTarget.RenderImage')
	def gettitle(self):
		return self._title
	def settitle(self, s):
		yagahost.SetScreenTitle(s)
		self._title = s
	title = property(gettitle, settitle)

class PrinterRenderTarget(IRenderTarget):
	def __init__(self):
		pass
	def RenderBegin(self, clearFlags):
		pass
	def RenderEnd(self):
		pass
	def RenderImage(self, imageBuffer, opacity, dstRect, srcRect):
		print('STUB: PrinterRenderTarget.RenderImage')

class PrintDevice(object):
	def __init__(self):
		pass
	def CreateRenderTarget(self):
		return PrinterRenderTarget()

class VideoDevice(object):
	def __init__(self):
		self.currentMode = VideoMode(640, 480, PixelFormat.PXL_R5G6B5)
		self.modes = [ self.currentMode ]
	def CreateRenderTarget(self, videoMode, buffers, targetType):
		self.currentMode = videoMode
		self.renderTarget = RenderTarget(videoMode, targetType)
		return self.renderTarget

class Camera(object):
	def __init__(self):
		pass

class Image(object):
	def __init__(self, w, h):
		self.pixelFmt = PixelFormat.PXL_R5G6B5
		self.imgData = [ 0 ] * (w * h)
		self.width = w
		self.height = h
	def Fill(self, color, r):
		pass
	def Composite(self, image, opacity, dstRect, srcRect):
		pass
	def Copy(self, image, dstRect, srcRect):
		pass

class GraphicsSystemImpl(object):
	def __init__(self):
		self.systemType = GraphicsHardware.SYSTEM_SOFTWARE
		self.primaryVideoDev = VideoDevice()
		self.videoDevices = [ self.primaryVideoDev ]
		self.primaryPrintDev = PrintDevice()
	def CreateCamera(self):
		self.camera = Camera()
		return self.camera
	def CreateImage(self, w, h, fmt):
		return Image(w, h)

g_graphicsSystem = GraphicsSystemImpl()

def GraphicsSystem():
	return g_graphicsSystem

class PngHandler(object):
	def __init__(self):
		pass

class MngHandler(object):
	def __init__(self):
		pass

class RleHandler(object):
	def __init__(self):
		pass

class TargaHandler(object):
	def __init__(self):
		pass

class CompressionType:
	COMPRESS_YRLE = 1

class ImageLayer(object):
	def __init__(self):
		self.image = Image(0, 0)

class ImageFrame(object):
	def __init__(self, num):
		self.layers = [ ImageLayer() ]

class IImageAnim(object):
	def __init__(self, res):
		self.res = res
		if res:
			count = yagahost.GetAnimationFramesCount(res.num)
			if count:
				self.frames = [ ImageFrame(x) for x in range(count) ]
				self.framesPerSecond = 0
			else:
				print('WARNING: res ' + str(res) + ' has no frame')
		else:
			print('WARNING: res is None')
	def Compress(self, compressionType, flag):
		print('STUB: IImageAnim.Compress')
		return self
	def __str__(self):
		return 'IImageAnim res:' + str(self.res)

class IImage(object):
	def __init__(self, img):
		self.img = img

def Color():
	return 0
