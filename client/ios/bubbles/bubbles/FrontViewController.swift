//
//  FrontViewController.swift
//  bubbles
//
//  Created by Valentin Palade on 06/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

import UIKit

class FrontViewController: UIViewController, UITextFieldDelegate {
    //MARK: Properties
    @IBOutlet weak var hostTextField: UITextField!
    @IBOutlet weak var compressSwitch: UISwitch!
    @IBOutlet weak var secureSwitch: UISwitch!
    @IBOutlet weak var autoSwitch: UISwitch!
    @IBOutlet weak var roomTextField: UITextField!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.
        hostTextField.delegate = self
        roomTextField.delegate = self
    }
    
    //MARK: UITextFieldDelegate
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        return true
    }
    func textFieldDidEndEditing(_ textField: UITextField) {
        //do nothing for now
    }
    
    //MARK: Segue
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        
        super.prepare(for: segue, sender: sender)
        
        switch(segue.identifier ?? "") {
            
        case "Proceed":
            guard let drawViewController = segue.destination as? DrawViewController else {
                fatalError("Unexpected destination: \(segue.destination)")
            }
            
            drawViewController.compress = compressSwitch.isOn
            drawViewController.secure = secureSwitch.isOn
            drawViewController.auto_pilot = autoSwitch.isOn
            drawViewController.host_name = hostTextField.text
            drawViewController.room_name = roomTextField.text
            
        default:
            fatalError("Unexpected Segue Identifier; \(segue.identifier)")
        }
    }
}

