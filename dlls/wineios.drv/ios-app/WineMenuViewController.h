/*
 * Wine iOS Menu View Controller
 *
 * Provides a UI for selecting and running Windows executables
 */

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol WineMenuDelegate <NSObject>
- (void)wineMenuDidSelectExecutableAtPath:(NSString *)path;
- (void)wineMenuDidRequestSettings;
@end

@interface WineMenuViewController : UIViewController <UITableViewDelegate, UITableViewDataSource, UIDocumentPickerDelegate>

@property (nonatomic, weak) id<WineMenuDelegate> delegate;
@property (nonatomic, strong, readonly) UITableView *tableView;

- (void)openFilePicker;

@end

NS_ASSUME_NONNULL_END
