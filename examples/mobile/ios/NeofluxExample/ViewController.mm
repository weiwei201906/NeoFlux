// =============================================================================
// NeoFlux iOS Example - ViewController.mm
//
// Demonstrates how to embed a NeoFlux rendering view in an iOS ViewController.
// Uses a CAEAGLLayer-backed UIView for OpenGL ES rendering, with the NeoFlux
// Application managing the widget tree and render loop.
// =============================================================================

#import "ViewController.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES3/gl.h>

#include <memory>

#include "neoflux/application.h"
#include "neoflux/widget/text.h"
#include "neoflux/widget/button.h"
#include "neoflux/widget/column.h"

@interface ViewController () {
  EAGLContext* _context;
  UIView* _glView;
  neoflux::Application* _app;
  CADisplayLink* _displayLink;
}
@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  // Create an OpenGL ES 3.0 context.
  _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (_context == nil) {
    // Fall back to OpenGL ES 2.0 if 3.0 is unavailable.
    _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  }
  [EAGLContext setCurrentContext:_context];

  // Create a full-screen view backed by a CAEAGLLayer.
  _glView = [[UIView alloc] initWithFrame:self.view.bounds];
  _glView.autoresizingMask = UIViewAutoresizingFlexibleWidth |
                             UIViewAutoresizingFlexibleHeight;
  CAEAGLLayer* eaglLayer = (CAEAGLLayer*)_glView.layer;
  eaglLayer.opaque = YES;
  eaglLayer.drawableProperties = @{
    kEAGLDrawablePropertyRetainedBacking : @NO,
    kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
  };
  [self.view addSubview:_glView];

  // Initialize the NeoFlux Application with the EAGL context.
  _app = new neoflux::Application();
  _app->SetFontDir:@"fonts".UTF8String);
  _app->Init(self.view.bounds.size.width, self.view.bounds.size.height,
             (__bridge void*)_context);

  // Build a simple widget tree.
  auto root = std::make_shared<neoflux::Column>();
  root->AddChild(std::make_shared<neoflux::Text>("Hello NeoFlux on iOS!"));

  auto button = std::make_shared<neoflux::Button>("Click Me");
  button->SetOnClick([]() {
    // Button click handler.
  });
  root->AddChild(button);

  _app->SetRoot(root);

  // Start the render loop using CADisplayLink (synced to screen refresh).
  _displayLink = [CADisplayLink displayLinkWithTarget:self
                                             selector:@selector(renderFrame:)];
  [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                     forMode:NSDefaultRunLoopMode];
}

- (void)renderFrame:(CADisplayLink*)displayLink {
  if (_app != nullptr) {
    _app->RunFrame();
  }
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  if (_app != nullptr) {
    _app->Resize(self.view.bounds.size.width, self.view.bounds.size.height);
  }
}

- (void)dealloc {
  [_displayLink invalidate];
  if (_app != nullptr) {
    delete _app;
    _app = nullptr;
  }
  [EAGLContext setCurrentContext:nil];
}

@end
