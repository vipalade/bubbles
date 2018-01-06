//
//  DrawViewController.swift
//  bubbles
//
//  Created by Valentin Palade on 06/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

import UIKit
import os.log

class DrawViewController: UIViewController {

    @IBOutlet weak var drawView: DrawView!
    
    var secure: Bool = false
    var compress: Bool = false
    var auto_pilot: Bool = false
    var host_name: String!
    var room_name: String!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        os_log("DrawViewController:secure = %@; compress = %@; auto_pilot = %@; hostname = %@; room_name = %@", log: OSLog.default, type: .debug, secure.description, compress.description, auto_pilot.description, host_name, room_name)
        
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        drawView?.onLoad()
    }
}
