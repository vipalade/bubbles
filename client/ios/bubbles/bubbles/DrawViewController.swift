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
        
        guard let ca_cert_path = Bundle.main.path(forResource: "bubbles-ca-cert", ofType: "pem") else {
            os_log("DrawViewController: ca-cert file not found", log: OSLog.default, type: .debug)
            return
        }
        guard let client_cert_path = Bundle.main.path(forResource: "bubbles-client-cert", ofType: "pem") else {
            os_log("DrawViewController: client-cert file not found", log: OSLog.default, type: .debug)
            return
        }
        guard let client_key_path = Bundle.main.path(forResource: "bubbles-client-key", ofType: "pem") else {
            os_log("DrawViewController: client-key file not found", log: OSLog.default, type: .debug)
            return
        }
        
        let ca_cert_txt: String! = try? String(contentsOfFile: ca_cert_path, encoding: String.Encoding.utf8)
        let client_cert_txt: String! = try? String(contentsOfFile: client_cert_path, encoding: String.Encoding.utf8)
        let client_key_txt: String! = try? String(contentsOfFile: client_key_path, encoding: String.Encoding.utf8)
        os_log("DrawViewController:ca_cert %@ client_cert %@ client_key %@", log: OSLog.default, type: .debug, ca_cert_txt, client_cert_txt, client_key_txt)
        let engine_rv = engine_start(host_name, room_name, secure ? 1:0, compress ? 1:0, auto_pilot ? 1:0, ca_cert_txt, client_cert_txt, client_key_txt)
        os_log("DrawViewController: engine_start rv = %@", log: OSLog.default, type: .debug, engine_rv.description)
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        drawView?.onLoad()
    }
}
