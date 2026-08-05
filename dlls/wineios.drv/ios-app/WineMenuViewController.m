/*
 * Wine iOS Menu View Controller Implementation
 *
 * Provides a UI for selecting and running Windows executables
 */

#import "WineMenuViewController.h"
#import "WineAppDelegate.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

@interface WineMenuViewController () <UIDocumentPickerDelegate>
{
    UITableView *_tableView;
    NSArray<NSArray<NSString *> *> *_menuItems;
    NSArray<NSString *> *> *_menuIcons;
}
@end

@implementation WineMenuViewController

@synthesize tableView = _tableView;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.title = @"Wine iOS";
    self.view.backgroundColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.15 alpha:1.0];
    
    [self setupMenuItems];
    [self setupUI];
}

- (void)setupMenuItems {
    _menuItems = @[
        @[@"Open EXE File", @"Open ZIP Archive", @"Recent Apps"],
        @[@"Wine Settings", @"Manage Prefix", @"File Manager"],
        @[@"About Wine iOS", @"Help & Documentation"]
    ];
}

- (void)setupUI {
    // Header view with Wine logo/title
    UIView *headerView = [[UIView alloc] init];
    headerView.frame = CGRectMake(0, 0, self.view.bounds.size.width, 180);
    
    // Wine title
    UILabel *titleLabel = [[UILabel alloc] init];
    titleLabel.text = @"Wine iOS";
    titleLabel.font = [UIFont systemFontOfSize:36 weight:UIFontWeightBold];
    titleLabel.textColor = [UIColor whiteColor];
    titleLabel.textAlignment = NSTextAlignmentCenter;
    titleLabel.frame = CGRectMake(0, 60, self.view.bounds.size.width, 50);
    [headerView addSubview:titleLabel];
    
    // Version label
    UILabel *versionLabel = [[UILabel alloc] init];
    versionLabel.text = @"Windows Compatibility Layer for iOS";
    versionLabel.font = [UIFont systemFontOfSize:14];
    versionLabel.textColor = [UIColor colorWithWhite:0.7 alpha:1.0];
    versionLabel.textAlignment = NSTextAlignmentCenter;
    versionLabel.frame = CGRectMake(0, 115, self.view.bounds.size.width, 25);
    [headerView addSubview:versionLabel];
    
    // Open File button (prominent)
    UIButton *openFileButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [openFileButton setTitle:@"  Open EXE or ZIP  " forState:UIControlStateNormal];
    [openFileButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    openFileButton.backgroundColor = [UIColor colorWithRed:0.2 green:0.5 blue:0.2 alpha:1.0];
    openFileButton.layer.cornerRadius = 12;
    openFileButton.titleLabel.font = [UIFont systemFontOfSize:18 weight:UIFontWeightSemibold];
    openFileButton.frame = CGRectMake(40, 145, self.view.bounds.size.width - 80, 50);
    [openFileButton addTarget:self action:@selector(openFilePicker) forControlEvents:UIControlEventTouchUpInside];
    [headerView addSubview:openFileButton];
    
    // Table view
    _tableView = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStyleInsetGrouped];
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.backgroundColor = [UIColor clearColor];
    _tableView.separatorStyle = UITableViewCellSeparatorStyleSingleLine;
    _tableView.separatorColor = [UIColor colorWithWhite:0.3 alpha:1.0];
    _tableView.tableHeaderView = headerView;
    _tableView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:_tableView];
    
    // Layout
    [NSLayoutConstraint activateConstraints:@[
        [_tableView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [_tableView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [_tableView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [_tableView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor]
    ]];
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return _menuItems.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return _menuItems[section].count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *cellIdentifier = @"MenuCell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:cellIdentifier];
    
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellIdentifier];
        cell.backgroundColor = [UIColor colorWithRed:0.15 green:0.15 blue:0.2 alpha:1.0];
        cell.textLabel.textColor = [UIColor whiteColor];
        cell.textLabel.font = [UIFont systemFontOfSize:17];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        
        UIImageView *accessoryImage = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"chevron.right"]];
        accessoryImage.tintColor = [UIColor colorWithWhite:0.5 alpha:1.0];
        cell.accessoryView = accessoryImage;
    }
    
    cell.textLabel.text = _menuItems[indexPath.section][indexPath.row];
    
    return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    switch (section) {
        case 0: return @"APPLICATIONS";
        case 1: return @"MANAGEMENT";
        case 2: return @"INFORMATION";
        default: return nil;
    }
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section {
    if ([view isKindOfClass:[UITableViewHeaderFooterView class]]) {
        UITableViewHeaderFooterView *header = (UITableViewHeaderFooterView *)view;
        header.textLabel.textColor = [UIColor colorWithWhite:0.5 alpha:1.0];
        header.textLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
    }
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    NSString *item = _menuItems[indexPath.section][indexPath.row];
    
    if ([item isEqualToString:@"Open EXE File"]) {
        [self openFilePicker];
    } else if ([item isEqualToString:@"Open ZIP Archive"]) {
        [self openZIPPicker];
    } else if ([item isEqualToString:@"Recent Apps"]) {
        [self showRecentApps];
    } else if ([item isEqualToString:@"Wine Settings"]) {
        [self showSettings];
    } else if ([item isEqualToString:@"About Wine iOS"]) {
        [self showAbout];
    } else {
        [self showAlert:@"Coming Soon" message:[NSString stringWithFormat:@"%@ will be available soon.", item]];
    }
}

#pragma mark - Actions

- (void)openFilePicker {
    NSArray *types = @[
        @"com.microsoft.windows.executable",
        @"public.exe",
        @"public.file-executable",
        @"com.winamp.mp3",  // fallback
        @"public.zip-archive"
    ];
    
    // Use UTType for modern API
    if (@available(iOS 14.0, *)) {
        NSMutableArray<UTType *> *utTypes = [NSMutableArray array];
        [utTypes addObject:UTTypeExecutable];
        [utTypes addObject:UTTypeZIPArchive];
        [utTypes addObject:UTTypeData];
        
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:utTypes];
        picker.delegate = self;
        picker.allowsMultipleSelection = NO;
        [self presentViewController:picker animated:YES completion:nil];
    } else {
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:types inMode:UIDocumentPickerModeOpen];
        picker.delegate = self;
        picker.allowsMultipleSelection = NO;
        [self presentViewController:picker animated:YES completion:nil];
    }
}

- (void)openZIPPicker {
    if (@available(iOS 14.0, *)) {
        NSArray<UTType *> *utTypes = @[UTTypeZIPArchive, UTTypeData];
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:utTypes];
        picker.delegate = self;
        picker.allowsMultipleSelection = NO;
        [self presentViewController:picker animated:YES completion:nil];
    } else {
        NSArray *types = @[@"public.zip-archive", @"com.pkware.zip-archive"];
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:types inMode:UIDocumentPickerModeOpen];
        picker.delegate = self;
        picker.allowsMultipleSelection = NO;
        [self presentViewController:picker animated:YES completion:nil];
    }
}

#pragma mark - UIDocumentPickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    if (urls.count == 0) return;
    
    NSURL *url = urls.firstObject;
    [url startAccessingSecurityScopedResource];
    
    NSString *path = url.path;
    
    // Copy to app's documents directory
    NSString *documentsDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString *wineAppsDir = [documentsDir stringByAppendingPathComponent:@"WineApps"];
    
    [[NSFileManager defaultManager] createDirectoryAtPath:wineAppsDir withIntermediateDirectories:YES attributes:nil error:nil];
    
    NSString *filename = [path lastPathComponent];
    NSString *destPath = [wineAppsDir stringByAppendingPathComponent:filename];
    
    // Remove existing if present
    [[NSFileManager defaultManager] removeItemAtPath:destPath error:nil];
    
    NSError *error = nil;
    [[NSFileManager defaultManager] copyItemAtPath:path toPath:destPath error:&error];
    
    [url stopAccessingSecurityScopedResource];
    
    if (error) {
        [self showAlert:@"Error" message:[NSString stringWithFormat:@"Failed to copy file: %@", error.localizedDescription]];
        return;
    }
    
    // Notify delegate
    if ([self.delegate respondsToSelector:@selector(wineMenuDidSelectExecutableAtPath:)]) {
        [self.delegate wineMenuDidSelectExecutableAtPath:destPath];
    }
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    // User cancelled
}

#pragma mark - Navigation

- (void)showRecentApps {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Recent Apps"
                                                                   message:@"No recent applications.\nOpen an EXE or ZIP to get started."
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)showSettings {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Wine Settings"
                                                                   message:@"Settings panel coming soon.\n\nOptions will include:\n• Wine prefix management\n• Graphics settings\n• Sound configuration\n• Network settings"
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)showAbout {
    NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"1.0";
    NSString *build = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] ?: @"1";
    
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"About Wine iOS"
                                                                   message:[NSString stringWithFormat:@"Wine iOS v%@ (%@)\n\nWine is a free, open-source compatibility layer that allows you to run Windows applications on iOS.\n\nCopyright © 2024 Wine Project", version, build]
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [alert addAction:[UIAlertAction actionWithTitle:@"More Info" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://www.winehq.org/"] options:@{} completionHandler:nil];
    }]];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)showAlert:(NSString *)title message:(NSString *)message {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

@end
