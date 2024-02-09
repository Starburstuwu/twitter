//
//  NELogController.swift
//  networkextension
//
//  Created by Igor Sorokin on 2/9/24.
//

import Foundation
import os.log

public func wg_log(_ type: OSLogType, staticMessage: StaticString) {
  Log.Record(date: Date(), level: Log.Record.Level(from: type), message: "\(staticMessage)").save(at: Log.neLogURL)
}

public func wg_log(_ type: OSLogType, message: String) {
  Log.Record(date: Date(), level: Log.Record.Level(from: type), message: message).save(at: Log.neLogURL)
}

public func log(_ type: OSLogType, message: String) {
  Log.Record(date: Date(), level: Log.Record.Level(from: type), message: message).save(at: Log.neLogURL)
}
