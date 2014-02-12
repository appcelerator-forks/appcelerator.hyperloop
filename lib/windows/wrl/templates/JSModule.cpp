<%- renderTemplate('jsc/templates/doc.ejs') %>
#include "JSModule.h"

static map<string, HyperloopJS*> modules;

HyperloopJS::~HyperloopJS() {
}

JSValueRef JSGetId(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception)
{
	HyperloopJS *module = (HyperloopJS *)JSObjectGetPrivate(object);
	return HyperloopToString(ctx, module->id);
}

JSValueRef JSGetFilename(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception)
{
	HyperloopJS *module = (HyperloopJS *)JSObjectGetPrivate(object);
	return HyperloopToString(ctx, module->filename);
}

JSValueRef JSGetParent(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception)
{
	HyperloopJS *module = (HyperloopJS*)JSObjectGetPrivate(object);
	if (module->parent != nullptr)
	{
		return HyperloopMakeJSObject(ctx, module->parent);
	}
	return JSValueMakeNull(ctx);
}

JSValueRef JSGetLoaded(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception)
{
	HyperloopJS *module = (HyperloopJS*)JSObjectGetPrivate(object);
	return JSValueMakeBoolean(ctx, module->loaded);
}

JSValueRef JSGetDirname(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception)
{
	/*HyperloopJS *module = (HyperloopJS *)JSObjectGetPrivate(object);
	NSString *dir = [module->filename stringByDeletingLastPathComponent];
	dir = [NSString stringWithFormat:@"./%@",dir];
	if ([dir hasSuffix:@"/"]==NO)
	{
		dir = [dir stringByAppendingString:@"/"];
	}
	return HyperloopToString(ctx, dir);*/
	return nullptr;
}

JSValueRef JSRequire(JSContextRef ctx, JSObjectRef function, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
	/*HyperloopJS *module = (HyperloopJS *)JSObjectGetPrivate(object);

	if (argumentCount != 1)
	{
		return HyperloopMakeException(ctx, "path must be a string", exception);
	}

	NSString *path = HyperloopToNSString(ctx, arguments[0]);
	HyperloopJS *js = HyperloopLoadJS(ctx, module, path, module->prefix);

	if (js == nullptr)
	{
		HyperloopMakeException(ctx, "cannot find module '" + path + "'", exception);
		JSStringRef codeProperty = JSStringCreateWithUTF8CString("code");
		JSStringRef msgProperty = JSStringCreateWithUTF8CString("MODULE_NOT_FOUND");
		JSObjectRef exceptionObject = JSValueToObject(ctx, *exception, 0);
		JSObjectSetProperty(ctx, exceptionObject, codeProperty, JSValueMakeString(ctx, msgProperty), 0, 0);
		JSStringRelease(codeProperty);
		JSStringRelease(msgProperty);
		return JSValueMakeUndefined(ctx);
	}

	return js->exports;*/
	return nullptr;
}

/**
  *called to do an async dispatch on the main thread
 */
JSValueRef JSDispatchAsync(JSContextRef ctx, JSObjectRef function, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
	HyperloopJS *module = (HyperloopJS*)JSObjectGetPrivate(object);

	if (argumentCount != 1)
	{
		return HyperloopMakeException(ctx, "function takes a callback as function to invoke on main thread", exception);
	}

	JSObjectRef callback = JSValueToObject(ctx, arguments[0], exception);
	if (!JSObjectIsFunction(ctx, callback))
	{
		return HyperloopMakeException(ctx, "callback must be a function", exception);
	}
	CHECK_EXCEPTION(ctx, *exception, module->prefix);

	JSObjectCallAsFunction(ctx, callback, object, 0, NULL, NULL);

	return JSValueMakeUndefined(ctx);
}

/**
 * called when a new JS object is created for this class
 */
void JSInitialize(JSContextRef ctx, JSObjectRef object)
{
	HyperloopJS *module = (HyperloopJS*)JSObjectGetPrivate(object);
	JSValueProtect(module->context, module->exports);
}

/**
 * called when the JS object is ready to be garbage collected
 */
void JSFinalize(JSObjectRef object)
{
	HyperloopJS *module = (HyperloopJS*)JSObjectGetPrivate(object);
	if (module != nullptr)
	{
		JSValueUnprotect(module->context, module->exports);
	}
}

static JSStaticValue StaticValueArrayForJS [] = {
	{ "id", JSGetId, 0, kJSPropertyAttributeReadOnly|kJSPropertyAttributeDontEnum },
	{ "filename", JSGetFilename, 0, kJSPropertyAttributeReadOnly|kJSPropertyAttributeDontEnum },
	{ "parent", JSGetParent, 0, kJSPropertyAttributeReadOnly|kJSPropertyAttributeDontEnum },
	{ "loaded", JSGetLoaded, 0, kJSPropertyAttributeReadOnly|kJSPropertyAttributeDontEnum },
	{ "__dirname", JSGetDirname, 0, kJSPropertyAttributeReadOnly },
	{ "__filename", JSGetFilename, 0, kJSPropertyAttributeReadOnly },
	{ 0, 0, 0, 0 }
};

static JSStaticFunction StaticFunctionArrayForJS [] = {
	{ "require", JSRequire, kJSPropertyAttributeReadOnly },
	{ "dispatch_async", JSDispatchAsync, kJSPropertyAttributeReadOnly },
	{ 0, 0, 0 }
};

//Class HyperloopPathToClass(NSString *path, NSString *prefix)
//{
//	NSString *modulename = [path stringByReplacingOccurrencesOfString:@".js" withString:@""];
//	modulename = [modulename stringByReplacingOccurrencesOfString:@"/" withString:@"_"];
//	modulename = [prefix stringByAppendingString:modulename];
//	return NSClassFromString(modulename);
//}

static JSClassDefinition classDef;
static JSClassRef classRef;

JSObjectRef HyperloopMakeJSObject(JSContextRef ctx, HyperloopJS *module)
{
	static bool init;
	if (!init)
	{
		init = true;
		JSClassDefinition classDef = kJSClassDefinitionEmpty;
		classDef.staticFunctions = StaticFunctionArrayForJS;
		classDef.staticValues = StaticValueArrayForJS;
		classDef.finalize = JSFinalize;
		classDef.initialize = JSInitialize;
		classRef = JSClassCreate(&classDef);
	}

	return JSObjectMake(ctx, classRef, (void *)module);
}

HyperloopJS *HyperloopLoadJSWithLogger(JSContextRef ctx, HyperloopJS *parent, String ^path, String ^prefix, JSObjectRef logger)
{
	/*OutputDebugString(@"HyperloopLoadJS called with ctx=%p, parent=%p, path=%@, prefix=%@", ctx, parent, path, prefix);

	if (!modules)
	{
		modules = [[NSMutableDictionary alloc] init];
	}

	// For the logic, we follow node.js logic here: http://nodejs.org/api/modules.html#modules_module_filename

	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *filepath = path;
	HyperloopJS *module = nullptr;
	NSString *modulekey = nullptr;

	if ([path hasPrefix:@"./"] || [path hasPrefix:@"/"] || [path hasPrefix:@"../"])
	{
		filepath = path;
		if (parent!=nullptr)
		{
			NSString *dir = [[parent filename] stringByDeletingLastPathComponent];
			filepath = [dir stringByAppendingPathComponent:path];
		}
		filepath = [[resourcePath stringByAppendingPathComponent:filepath] stringByStandardizingPath];
		if ([filepath length] <= [resourcePath length])
		{
			// they have tried to ../ passed top of the root, just return nullptr
			return nullptr;
		}
		filepath = [filepath substringFromIndex:[resourcePath length]+1];

		if ((module = [modules objectForKey:[filepath stringByDeletingPathExtension]]))
		{
			return module;
		}
	}
	else if (parent == nullptr)
	{
		// not a specific path, must look at node_modules according to node spec (step 3)
		filepath = [@"./node_modules" stringByAppendingPathComponent:path];
		if ((module = [modules objectForKey:[filepath stringByDeletingPathExtension]]))
		{
			return module;
		}
		return HyperloopLoadJS(ctx, parent, filepath, prefix);
	}

	Class cls = HyperloopPathToClass(filepath, prefix);

	DEBUGLOG(@"HyperloopLoadJS::HyperloopPathToClass filepath=%@, prefix=%@, cls=%@",filepath,prefix,cls);

	if (cls == nullptr)
	{
		// check to see if a directory with package.json
		NSString *subpath = [path stringByAppendingPathComponent:@"/package.json"];
		NSString *packagePath = [resourcePath stringByAppendingPathComponent:subpath];
		BOOL isDirectory = NO;
		if ([fileManager fileExistsAtPath:packagePath isDirectory:&isDirectory] && !isDirectory)
		{
			NSError *error = nullptr;
			NSString *fileContents = [NSString stringWithContentsOfFile:packagePath encoding:NSUTF8StringEncoding error:&error];
			NSData *fileData = [fileContents dataUsingEncoding:NSUTF8StringEncoding];
			NSDictionary *json = [NSJSONSerialization JSONObjectWithData:fileData options:0 error:&error];
			if (error == nullptr)
			{
				// look for main field in JSON
				NSString *main = [json objectForKey:@"main"];
				if (main != nullptr)
				{
					subpath = [path stringByAppendingPathComponent:main];
					packagePath = [[resourcePath stringByAppendingPathComponent:subpath] stringByStandardizingPath];
					filepath = [packagePath substringFromIndex:[resourcePath length]+1];
					if ((module = [modules objectForKey:[filepath stringByDeletingPathExtension]]))
					{
						return module;
					}
					cls = HyperloopPathToClass(filepath,prefix);
				}
			}
		}
		if (cls == nullptr)
		{
			// look for index.js
			subpath = [path stringByAppendingPathComponent:@"/index.js"];
			packagePath = [[resourcePath stringByAppendingPathComponent:subpath] stringByStandardizingPath];
			filepath = [packagePath substringFromIndex:[resourcePath length]+1];
			if ((module = [modules objectForKey:[filepath stringByDeletingPathExtension]]))
			{
				return module;
			}
			cls = HyperloopPathToClass(filepath,prefix);
		}

		// if we're already inside node_modules, don't go into this block or you'll have infinite recursion
		if (cls == nullptr && [path rangeOfString:@"node_modules/"].location == NSNotFound)
		{
			// check node modules, by walking up from the current directory to the top of the directory
			NSString *top = parent ? [parent.filename stringByDeletingLastPathComponent] : @"";
			while (top != nullptr)
			{
				NSString *fp = [top stringByAppendingPathComponent:[NSString stringWithFormat:@"node_modules/%@",path]];
				if ((module = [modules objectForKey:[fp stringByDeletingPathExtension]]))
				{
					return module;
				}
				module = HyperloopLoadJS(ctx,parent,fp,prefix);
				if (module != nullptr)
				{
					return module;
				}
				if ([top length] == 0)
				{
					// already at the end, now break
					break;
				}
				top = [top stringByDeletingLastPathComponent];
			}
		}
	}
	else
	{
		// this is a class module, make sure the module key is unique
		modulekey = [filepath stringByAppendingString:prefix];

		// check and see if we have already loaded the module, then
		// go ahead and return it
		if ((module=[modules objectForKey:modulekey]))
		{
			return module;
		}
	}

	if (cls != nullptr)
	{
		// make sure we strip it, since we're going to add it below
		filepath = [filepath stringByDeletingPathExtension];

		DEBUGLOG(@"HyperloopLoadJS::cls!=nullptr filepath=%@",filepath);

		HyperloopJS *module = [HyperloopJS new];
		module->id = [path hasPrefix:@"./"] ? [path substringFromIndex:2] : path;
		module->filename = [filepath stringByAppendingPathExtension:@"js"];
		module->loaded = NO;
		module->parent = parent;
		module->context = HyperloopGetGlobalContext(ctx);
		module->exports = JSObjectMake(ctx, 0, 0);
		module->prefix = prefix;

		if (modulekey == nullptr)
		{
			modulekey = filepath;
		}

		[modules setObject:module forKey:modulekey];

		JSObjectRef moduleObjectRef = HyperloopMakeJSObject(ctx,module);
		JSStringRef exportsProperty = JSStringCreateWithUTF8CString("exports");
		JSObjectSetProperty(ctx, moduleObjectRef, exportsProperty, module->exports, 0, 0);
		JSStringRelease(exportsProperty);

		// install our own logger
		if (logger != nullptr)
		{
			JSStringRef consoleProperty = JSStringCreateWithUTF8CString("console");
			JSObjectSetProperty(ctx, moduleObjectRef, consoleProperty, logger, 0, 0);
			JSStringRelease(consoleProperty);
		}

		// load up our JS
		Class <HyperloopModule> mcls = (Class<HyperloopModule>)cls;
		DEBUGLOG(@"HyperloopLoadJS::mcls %@",mcls);

		// load up our context
		[mcls load:ctx withObject:JSContextGetGlobalObject(ctx)];

		NSData *compressedBuf = [mcls buffer];

		// if empty, just skip the JS invocation
		if ([compressedBuf length] > 1)
		{

			// load up our properties that we want to expose
			JSPropertyNameArrayRef properties = JSObjectCopyPropertyNames(ctx, moduleObjectRef);
			NSMutableArray *propertyNames = [NSMutableArray array];
			size_t count = JSPropertyNameArrayGetCount(properties);

			JSStringRef parameterNames[1];
			JSValueRef arguments[1];

			parameterNames[0] = JSStringCreateWithUTF8CString("module");
			arguments[0]=moduleObjectRef;

			// loop through and put module related variables in a wrapper scope
			for (size_t c = 0;c<count;c++)
			{
				JSStringRef propertyName = JSPropertyNameArrayGetNameAtIndex(properties,c);
				size_t buflen = JSStringGetMaximumUTF8CStringSize(propertyName);
				char buf[buflen];
				buflen = JSStringGetUTF8CString(propertyName, buf, buflen);
				buf[buflen] = '\0';
				JSValueRef paramObject = JSObjectGetProperty(ctx, moduleObjectRef, propertyName, 0);
				BOOL added = NO;
				if (JSValueIsObject(ctx,paramObject))
				{
					JSStringRef script = JSStringCreateWithUTF8CString([[NSString stringWithFormat:@"(typeof(this.%s)==='function')",buf] UTF8String]);
					JSValueRef result = JSEvaluateScript(ctx,script,moduleObjectRef,NULL,0,0);
					if (JSValueToBoolean(ctx,result))
					{
						// make sure that the right scope (this object) is set for the function
						[propertyNames addObject:[NSString stringWithFormat:@"%s=function %s(){return $self.%s.apply($self,arguments)}",buf,buf,buf]];
						added = YES;
					}
					JSStringRelease(script);
				}
				if (added==NO)
				{
					[propertyNames addObject:[NSString stringWithFormat:@"%s=this.%s",buf,buf]];
				}
				JSStringRelease(propertyName);
			}

			NSData *buffer = HyperloopDecompressBuffer(compressedBuf);
			NSString *jscode = [[[NSString alloc] initWithData:buffer encoding:NSUTF8StringEncoding] autorelease];
			NSString *wrapper = [NSString stringWithFormat:@"var $self = this, %@;\n%@;",[propertyNames componentsJoinedByString:@", "],jscode];

			JSStringRef fnName = JSStringCreateWithUTF8CString("require");
			JSStringRef body = JSStringCreateWithUTF8CString([wrapper UTF8String]);

			JSValueRef *exception = NULL;
			JSStringRef filename = JSStringCreateWithUTF8CString(
				[[path stringByAppendingPathExtension:@"js"] UTF8String]);
			JSObjectRef requireFn = JSObjectMakeFunction(ctx, fnName, 1, parameterNames, body, filename, 1, &exception);
			JSStringRelease(filename);
			CHECK_EXCEPTION(ctx,exception,module->prefix);

			JSValueRef fnResult = JSObjectCallAsFunction(ctx, requireFn, moduleObjectRef, 1, arguments, &exception);
			JSStringRef pathRef = JSStringCreateWithUTF8CString([path UTF8String]);
			JSStringRef prefixRef = JSStringCreateWithUTF8CString([prefix UTF8String]);

			JSValueRef args[3];
			args[0] = exception;
			args[1] = JSValueMakeString(ctx,pathRef);
			args[2] = JSValueMakeString(ctx,prefixRef);

			JSStringRelease(pathRef);
			JSStringRelease(prefixRef);
			JSStringRelease(fnName);
			JSStringRelease(body);
			JSStringRelease(parameterNames[0]);
		}

		module->loaded = YES;

		// we need to pull the exports in case it got assigned (such as setting a Class to exports)
		JSStringRef exportsProp = JSStringCreateWithUTF8CString("exports");
		JSValueRef exportsValueRef = JSObjectGetProperty(ctx, moduleObjectRef, exportsProp, 0);
		if (JSValueIsObject(ctx, exportsValueRef))
		{
			module->exports = JSValueToObject(ctx, exportsValueRef, 0);
		}
		JSStringRelease(exportsProp);

		return module;
	}*/

	return nullptr;
}

HyperloopJS *HyperloopLoadJS(JSContextRef ctx, HyperloopJS *parent, String ^path, String ^prefix)
{
	return HyperloopLoadJSWithLogger(ctx, parent, path, prefix, nullptr);
}