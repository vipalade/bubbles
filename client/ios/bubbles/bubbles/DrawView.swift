//
//  DrawView.swift
//  bubbles
//
//  Created by Valentin Palade on 06/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

import UIKit
import os.log

class DrawView: UIView {
    var position: CGPoint = CGPoint(x: 0, y: 0)
    let diameter: CGFloat = 30.0
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    required init?(coder: NSCoder){
        super.init(coder: coder)
        position = super.center
    }
    
    public func onLoad() {
        position = super.center
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        position = touches.first!.location(in: self)
        os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        self.setNeedsDisplay()
    }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        position = touches.first!.location(in: self)
        os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        self.setNeedsDisplay()
    }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        position = touches.first!.location(in: self)
        os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        self.setNeedsDisplay()
    }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        //nothing here yet
    }
    
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        // Drawing code
        var path = UIBezierPath()
        path = UIBezierPath(ovalIn: CGRect(x: position.x - diameter/2, y: position.y - diameter/2, width: diameter, height: diameter))
        UIColor.black.setStroke()
        UIColor.black.setFill()
        path.lineWidth = 5
        path.stroke()
        path.fill()
        
    }

}
