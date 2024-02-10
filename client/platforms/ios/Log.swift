//
//  Log.Record.swift
//  AmneziaVPN
//
//  Created by Igor Sorokin on 2/9/24.
//

import Foundation
import os.log

struct Log {
  static let IsLoggingEnabledKey = "IsLoggingEnabled"
  
  private static let appGroupID = {
#if os(iOS)
    "group.org.amnezia.AmneziaVPN"
#elseif os(macOS)
    "group.org.amnezia.AmneziaVPN"
#endif
  }()
  
  static let neLogURL = {
    let sharedContainerURL = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: appGroupID)!
    return sharedContainerURL.appendingPathComponent("ne.log", isDirectory: false)
  }()
  
  static var sharedUserDefaults = {
    UserDefaults(suiteName: appGroupID)!
  }()
  
  static let dateFormatter: DateFormatter = {
      let dateFormatter = DateFormatter()
      dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
      return dateFormatter
  }()

  var records: [Record]
  
  init() {
    self.records = []
  }
  
  init(_ str: String) {
    self.records = str.split(whereSeparator: \.isNewline)
      .compactMap {
        Record(String($0))!
      }
  }
  
  init?(at url: URL) {
    if !FileManager.default.fileExists(atPath: url.path) {
      guard let _ = try? "".data(using: .utf8)!.write(to: url) else { return nil }
    }
    
    guard let fileHandle = try? FileHandle(forUpdating: url) else { return nil }
    
    defer { fileHandle.closeFile() }
    
    guard
      let data = try? fileHandle.readToEnd(),
      let str = String(data: data, encoding: .utf8) else {
      return nil
    }
    
    self.init(str)
  }
  
  static func clear(at url: URL) {
    if FileManager.default.fileExists(atPath: url.path) {
      guard let fileHandle = try? FileHandle(forUpdating: url) else { return }
      
      defer { fileHandle.closeFile() }
      
      fileHandle.truncateFile(atOffset: 0)
    }
  }
}

extension Log: CustomStringConvertible {
  var description: String {
    records
      .map {
        $0.description
      }
      .joined(separator: "\n")
  }
}
