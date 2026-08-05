/*
 * Wine iOS Application
 *
 * Copyright 2024 Wine Project
 */

#import <UIKit/UIKit.h>
#import "WineAppDelegate.h"

int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    @autoreleasepool {
        appDelegateClassName = NSStringFromClass([WineAppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
