
import struct
import time
import yagahost

class KeyCodes:
	KEY_CODES_BEGIN = yagahost.KEY_CODES_BEGIN
	KEY_ENTER = yagahost.KEY_ENTER
	KEY_ESCAPE = yagahost.KEY_ESCAPE
	KEY_UP = yagahost.KEY_UP
	KEY_DOWN = yagahost.KEY_DOWN
	KEY_LEFT = yagahost.KEY_LEFT
	KEY_RIGHT = yagahost.KEY_RIGHT
	KEY_CONTROL = yagahost.KEY_CONTROL
	KEY_SHIFT = yagahost.KEY_SHIFT
	KEY_BACKSPACE = yagahost.KEY_BACKSPACE
	KEY_F1 = yagahost.KEY_F1
	KEY_F2 = yagahost.KEY_F2
	KEY_F3 = yagahost.KEY_F3
	KEY_F4 = yagahost.KEY_F4
	KEY_F5 = yagahost.KEY_F5
	KEY_F6 = yagahost.KEY_F6
	KEY_F7 = yagahost.KEY_F7
	KEY_F8 = yagahost.KEY_F8
	KEY_F9 = yagahost.KEY_F9

class EEventClass:
	CLASS_RENDER_TARGET = 1
	CLASS_MOUSE = 2
	CLASS_KEYBOARD = 4
	CLASS_GAMEPAD = 8
	CLASS_TIMER = 16
	CLASS_EVENT_STREAM = 32

class ETimerEvent:
	TIMER_TICK = 1

class EventSourceFlags:
	IFLAGS_RAW_BUTTONS = 1 << 0
	IFLAGS_TRANSLATE_BUTTONS = 1 << 1

class ERenderTargetEvent:
	IEVENT_RT_CLOSE = 1
	IEVENT_RT_MODE_TOGGLE = 2
	IEVENT_RT_ACTIVATED = 3

class EInputEvent:
	IEVENT_AXIS_POS_X = 1
	IEVENT_AXIS_POS_Y = 2
	IEVENT_BUTTON_PRESS = 3
	IEVENT_BUTTON_DOWN = 3 # same code as _PRESS
	IEVENT_BUTTON_UP = 4

class ETimerEvent:
	TIMER_TICK = 1

class ERecieverReturn:
	EVENT_HANDLED = 1
	EVENT_NOT_HANDLED = 2

class EvtHandler(object):
	def __init__(self):
		pass

class EvbElement(object):
	def __init__(self, name, attrs):
		self.name = name
		self.attrs = attrs

class EvbAttribute(object):
	def __init__(self, name, value):
		self.name = name
		self.value = value

class EvbData(object):
	def __init__(self):
		self.data = None
	def Load(self, f):
		size = struct.unpack("<I", f.read(4))[0]
		count = struct.unpack("<I", f.read(4))[0]
		data = []
		for i in range(count):
			pos = struct.unpack("<f", f.read(4))[0]
			c = struct.unpack("<I", f.read(4))[0]
			if c == 0x7ab7:
				a, b = self.Load_7ab7(f)
			else:
				assert c == 0x3c8c
				a, b = self.Load_3c8c(f)
			data.append( (pos, c, a, b) )
		self.data = data
	def GetData(self, num):
		if num >= 0 and num < len(self.data):
			return self.data[num]
		return None
	def Load_7ab7(self, f):
		mask = struct.unpack("<I", f.read(4))[0]
		b = struct.unpack("<I", f.read(4))[0]
		assert b == 0x3f800000 # 1.0
		unk = struct.unpack("<I", f.read(4))[0]
		d = struct.unpack("<I", f.read(4))[0]
		assert d == 0
		e = struct.unpack("<I", f.read(4))[0]
		assert e == 0
		return (mask, unk)
	def Load_3c8c(self, f):
		d = struct.unpack("<I", f.read(4))[0]
		assert d == 0 or d == 1
		e = struct.unpack("<I", f.read(4))[0]
		assert e == 0x3f800000 # 1.0
		event_type = f.read(4) # 0x3f800000 | mati | ckpo
		g = struct.unpack("<I", f.read(4))[0]
		element_count = struct.unpack("<I", f.read(4))[0]
		elements = []
		for j in range(element_count):
			element_name_len = struct.unpack("<I", f.read(4))[0]
			attribute_count = struct.unpack("<I", f.read(4))[0]
			element_name = f.read(element_name_len + 1)
			attributes = []
			for i in range(attribute_count):
				attribute_name_len = struct.unpack("<I", f.read(4))[0]
				attribute_data_len = struct.unpack("<I", f.read(4))[0]
				attribute_name = f.read(attribute_name_len + 1)
				attribute_data = f.read(attribute_data_len + 1)
				attributes.append(EvbElement(attribute_name, attribute_data))
			elements.append(EvbElement(element_name, attributes))
		return (event_type, elements)

class EvbHandler(object):
	def __init__(self):
		pass

class IEventReciever(object):
	def __init__(self):
		self._eventsMask = 0 # EEventClass.CLASS_*
	def Raise(self, event):
		pass

class IEventSource(object):
	def __init__(self, renderTarget):
		self.handled = False
		self.flags = 0

class IEventStream(object):
	def __init__(self, res):
		self._ev = EvbData()
		self._ev.Load(res)
		self._time = 0.
	def Run(self, scene):
		# print('STUB: IEventStream.Run res:' + str(self.res))
		self._time = 0.
	def Seek(self, pos):
		# print('STUB: IEventStream.Seek res:' + str(self.res) + ' pos:' + str(pos) + ' ' + str(self.stream))
		self._time += pos

class EventStreamPlayback(object):
	def __init__(self, name):
		self._name = name
		self.stream = None
		self.identity = self
		self._time = 0.
	def Run(self, scene):
		# print('STUB: EventStreamPlayback.Run stream:' + str(self.stream))
		self._time = 0.
	def Seek(self, pos):
		# print('STUB: EventStreamPlayback.Seek stream:' + str(self.stream) + ' pos:' + str(pos))
		self._time += pos
	def EventData(self, num):
		# print('STUB: EventStreamPlayback.EventData num:' + str(num))
		data = self.stream._ev.GetData(num)
		if data:
			pos, c, event_type, event_elements = data
			assert c == 0x3c8c
			return event_elements
		return None
	def MaskData(self, num):
		data = self.stream._ev.GetData(num)
		if data:
			pos, c, mask, unk = data
			assert c == 0x7ab7
			return mask
		return None

class Event(object):
	def __init__(self, eventClass, eventType, value = 0):
		self.eventClass = eventClass
		self.eventType = eventType
		self.value = value
		self.deviceID = 0 # targetRender.identity
		self.elementID = 0 # keycode

class IMouseInputDevice(object):
	def __init__(self, device):
		pass
	def SetPosition(self, x, y):
		pass

class Timer(object):
	def __init__(self):
		self.tickFrequency = 0
		self.eventReciever = None
		self._nextTick = 0

class EventManagerImpl(object):
	def __init__(self):
		self.timers = []
		self.events = []
		self._quit = False
	def RegisterEventReciever(self, seq, recv):
		print('STUB: EventManagerImpl.RegisterEventReceiver ' + str(recv))
		self.events.append(recv)
		if seq > 10000:
			assert seq == 15500 # TYPE_ANIMATION_EVENT
		else:
			recv._eventsMask = seq
	def UnregisterEventReciever(self, recv):
		# print('STUB: EventManagerImpl.UnregisterEventReceiver ' + str(recv))
		self.events.remove(recv)
	def GetEventSource(self, evClass, index):
		# print('STUB: EventManagerImpl.GetEventSource eventClass:' + str(evClass) + ' index:' + str(index))
		return IEventSource(None)
	def CreateTimer(self):
		return Timer()
	def InstallTimer(self, timer):
		# print('STUB: EventManagerImpl.InstallTimer ' + str(timer))
		assert timer.tickFrequency > 0
		timer._nextTick = time.time() * 1000 + 1000 / timer.tickFrequency
		self.timers.append(timer)
	def StartEventLoop(self):
		# print('STUB: EventManagerImpl.StartEventLoop')
		while not self._quit:
			while True:
				event = yagahost.PollEvent()
				if not event:
					break
				#print(event)
				if event['type'] == yagahost.EVENT_QUIT:
					ev = Event(EEventClass.CLASS_RENDER_TARGET, ERenderTargetEvent.IEVENT_RT_CLOSE)
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_RENDER_TARGET) != 0:
							recvr.Raise(ev)
				elif event['type'] == yagahost.EVENT_MOUSE_MOTION:
					evx = Event(EEventClass.CLASS_MOUSE, EInputEvent.IEVENT_AXIS_POS_X, event['x'])
					evy = Event(EEventClass.CLASS_MOUSE, EInputEvent.IEVENT_AXIS_POS_Y, event['y'])
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_MOUSE) != 0:
							recvr.Raise(evx)
							recvr.Raise(evy)
				elif event['type'] == yagahost.EVENT_MOUSE_BUTTON_DOWN:
					ev = Event(EEventClass.CLASS_MOUSE, EInputEvent.IEVENT_BUTTON_DOWN)
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_MOUSE) != 0:
							recvr.Raise(ev)
				elif event['type'] == yagahost.EVENT_MOUSE_BUTTON_UP:
					ev = Event(EEventClass.CLASS_MOUSE, EInputEvent.IEVENT_BUTTON_UP)
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_MOUSE) != 0:
							recvr.Raise(ev)
				elif event['type'] == yagahost.EVENT_KEY_DOWN:
					ev = Event(EEventClass.CLASS_KEYBOARD, EInputEvent.IEVENT_BUTTON_DOWN)
					ev.elementID = event['code']
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_KEYBOARD) != 0:
							recvr.Raise(ev)
				elif event['type'] == yagahost.EVENT_KEY_UP:
					ev = Event(EEventClass.CLASS_KEYBOARD, EInputEvent.IEVENT_BUTTON_UP)
					ev.elementID = event['code']
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_KEYBOARD) != 0:
							recvr.Raise(ev)
				elif event['type'] == yagahost.EVENT_WINDOW_FOCUS:
					ev = Event(EEventClass.CLASS_RENDER_TARGET, ERenderTargetEvent.IEVENT_RT_ACTIVATED, event['focus'])
					for recvr in self.events:
						if (recvr._eventsMask & EEventClass.CLASS_RENDER_TARGET) != 0:
							recvr.Raise(ev)
			for recvr in self.events:
				if hasattr(recvr, 'streamPlayback'):
					# print('streamPlayback AnimRecv data:' + recvr.data + ' name:' + recvr.name)
					# TODO: check time
					ev = Event(EEventClass.CLASS_EVENT_STREAM, 0)
					ev.deviceID = recvr.streamPlayback
					if recvr.Raise(ev) == ERecieverReturn.EVENT_NOT_HANDLED:
						pass
			now = time.time() * 1000
			for timer in self.timers:
				if timer._nextTick <= now:
					diff = now - (timer._nextTick - 1000 / timer.tickFrequency)
					if timer.eventReciever:
						ev = Event(EEventClass.CLASS_TIMER, ETimerEvent.TIMER_TICK)
						ev.value = diff
						timer.eventReciever.Raise(ev)
					timer._nextTick = now + 1000 / timer.tickFrequency
			time.sleep(50. / 1000)
	def StopEventLoop(self):
		# print('STUB: EventManagerImpl.StopEventLoop')
		self._quit = True
	def CreateEventStreamPlayback(self, name):
		# print('STUB: EventManagerImpl.CreateEventStreamPlayback name:' + name)
		return EventStreamPlayback(name)
	def SuspendLocalTime(self):
		print('STUB: EventManagerImpl.SuspendLocalTime')
	def ResumeLocalTime(self):
		print('STUB: EventManagerImpl.ResumeLocalTime')

g_eventManager = EventManagerImpl()

def EventManager():
	return g_eventManager
